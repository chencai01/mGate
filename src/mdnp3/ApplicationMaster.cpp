#include "mdnp3/pch.h"
#include "mdnp3/ApplicationMaster.h"

namespace Dnp3Master
{
ApplicationMaster::ApplicationMaster()
{
    _fragment_rcv_event_handler = new FragmentDataEventHandler();
    _fragment_rcv_event_handler->connect(this, &ApplicationMaster::on_fragment_received);
    dnp3_point_rcv_event = NULL;
    dnp3_cmd_echo_event = NULL;
    dnp3_time_delay_event = NULL;

    _time_cto = 0;
    _time_delay = 0;
    _send_fragment_len = 0;
    _lastfrag_index = 0;

    _ap_oper_status = APOpStatusCode::Idle;
    _ap_uns_oper_status = APUNSOpStatusCode::Idle;
    _debug = false;
    _debug_stream = NULL;
}

ApplicationMaster::~ApplicationMaster()
{
    delete _fragment_rcv_event_handler;
}

void ApplicationMaster::connect_to_transport(TransportMaster* tp)
{
    _transport_loader = tp;
    _transport_loader->fragment_received_event = _fragment_rcv_event_handler;
}

void ApplicationMaster::on_fragment_received(void* sender, FragmentDataEventArg* e)
{
    _rcv_new_header.AC = e->fragment->at(0);
    _rcv_new_header.FC = (APFunctionCode::APFunctionCode)(e->fragment->at(1));
    _rcv_new_header.IIN.set_IIN(e->fragment->at(2), e->fragment->at(3));

    if (_debug)
    {
        *_debug_stream << "[Application]<****";
        *_debug_stream << std::hex << std::setfill('0');
        for (size_t i = 0; i < e->fragment->size(); i++)
            *_debug_stream << " " << std::setw(2) << (int)(e->fragment->at(i));

        *_debug_stream
                << std::dec     << "\n[Application header response]:\n"
                << "FIR: "      << _rcv_new_header.AC.get_fir()
                << ", FIN: "    << _rcv_new_header.AC.get_fin()
                << ", SEQ: "    << (unsigned short)_rcv_new_header.AC.get_seq()
                << ", CON: "    << _rcv_new_header.AC.get_con()
                << ", UNS: "    << _rcv_new_header.AC.get_uns()
                << ", IIN: "    << IINFlags::to_string(_rcv_new_header.IIN.flag_IIN)
                << ", FC: "     << APFunctionCode::to_string(_rcv_new_header.FC)
                << "\n";
    }

    // Received fragment is RESPONSE
    if (_rcv_new_header.FC == APFunctionCode::RESPONSE)
    {
        /*
        Khong stop timer o day.
        Tranh truong hop ied gui khong du fragment => master cho mai mai
        (bug nay duoc phat hien ngay 3/10/2016)
        Chi stop timer khi da nhan du fragment va chuyen sang proc_asdu()
        Trong qua trinh phan tich fragment se thuc hien reset timer de doi
        nhan fragment moi.
        */
        //_rsp_timeout.stop();
        switch (_ap_oper_status)
        {
        case APOpStatusCode::AwaitFirst:
            proc_frag_awaitfirst_state(e->fragment, e->length);
            break;
        case APOpStatusCode::Assembly:
            proc_frag_assembly_state(e->fragment, e->length);
            break;
        default:
            break;
        }
        return;
    }

    // Received fragment is UNSOLICITED RESPONSE
    if (_rcv_new_header.FC == APFunctionCode::UNSOLICITED_RESPONSE)
    {
        switch (_ap_uns_oper_status)
        {
        case APUNSOpStatusCode::Startup:
            proc_frag_uns_startup_state(e->fragment, e->length);
            break;
        case APUNSOpStatusCode::FirstUR:
            proc_frag_uns_first_ur_state(e->fragment, e->length);
            break;
        case APUNSOpStatusCode::Idle:
            proc_frag_uns_idle_state(e->fragment, e->length);
            break;
        default:
            break;
        }
        return;
    }
}

void ApplicationMaster::act_on_timeout()
{
    reset_layer_state();
}

void ApplicationMaster::reset_layer_state()
{
    _ap_oper_status = APOpStatusCode::Idle;
    _rsp_timeout.stop();
}

void ApplicationMaster::send_debug()
{
    *_debug_stream << "[Application]****>";
    *_debug_stream << std::hex << std::setfill('0');
    for (int i = 0; i < _send_fragment_len; i++)
        *_debug_stream << " " << std::setw(2) << (int)_send_fragment[i];

    *_debug_stream
            << std::dec     << "\n[Application header request]:\n"
            << "FIR: "      << _send_header.AC.get_fir()
            << ", FIN: "    << _send_header.AC.get_fin()
            << ", SEQ: "    << (unsigned short)_send_header.AC.get_seq()
            << ", CON: "    << _send_header.AC.get_con()
            << ", UNS: "    << _send_header.AC.get_uns()
            << ", FC: "     << APFunctionCode::to_string(_send_header.FC)
            << "\n";
}

/*********PROCESS FRAGMENT BASED ON OPERATION STATE OF APPLICATION LAYER*********/


void ApplicationMaster::proc_frag_awaitfirst_state(ByteVector* buff, ByteVector::size_type length)
{
    if (_rcv_new_header.AC.get_fir()) // FIR = 1, case 4,5,6
    {
        if (_rcv_new_header.AC.get_seq() == _send_header.AC.get_seq()) // N1 = N2, case 4,6
        {
            _rcv_header.AC = _rcv_new_header.AC;
            _rcv_header.FC = _rcv_new_header.FC;
            _rcv_header.IIN = _rcv_new_header.IIN;

            // Send confirm if requested
            if (_rcv_new_header.AC.get_con())
            {
                send_confirmed(_rcv_new_header.AC.get_seq(), false);
            }

            _asdu.clear();
            _lastfrag_index = 0; // Cap nhat index cua fragment cuoi cung
            if (_rcv_new_header.AC.get_fin()) // case 6, FIN = 1, accept fragment and process fragment
            {
                _ap_oper_status = APOpStatusCode::Idle;
                for (ByteVector::size_type i = 4; i < length; i++)
                {
                    _asdu.push_back(buff->at(i));
                }
                proc_asdu();
            }
            else // case 4, FIN = 0, accept fragment and start received timer
            {
                _ap_oper_status = APOpStatusCode::Assembly;
                for (ByteVector::size_type i = 4; i < length; i++)
                {
                    _asdu.push_back(buff->at(i));
                }
                _rsp_timeout.start();
            }
        }
        else
        {
            _rsp_timeout.start();    // case 5, N1 != N2
        }
    }
    else
    {
        _rsp_timeout.start();    // case 3, FIR = 0
    }
}

void ApplicationMaster::proc_frag_assembly_state(ByteVector* buff, ByteVector::size_type length)
{
    if (_rcv_new_header.AC.get_byte() == _rcv_header.AC.get_byte()) // case 8, 14 FIR, FIN, SEQ match previous
    {
        /*
        Compare octet-by-octet with previous accepted fragment
        If octets match, send confirm if requested, take no further action, and start receive timer
        if octets do not match, discard fragment and do not confirm
        */
        int __lastfrag_len = _asdu.size() - _lastfrag_index;
        if ((int)(length - 4) != __lastfrag_len)
        {
            _ap_oper_status = APOpStatusCode::Idle;
            return;
        }

        int __idx = _lastfrag_index - 4;
        for (ByteVector::size_type i = 4; i < length; i++)
        {
            if (_asdu.at(__idx + i) != buff->at(i))
            {
                _ap_oper_status = APOpStatusCode::Idle;
                return;
            }
        }

        // Octets matched
        // Send confirm if requested
        if (_rcv_new_header.AC.get_con())
        {
            send_confirmed(_rcv_new_header.AC.get_seq(), false);
        }

        // Start receive timer
        _rsp_timeout.start();
    }
    else if(_rcv_new_header.AC.get_seq() == _rcv_header.AC.get_next_seq()) // case 10, 12, SEQ = N + 1
    {
        if (!_rcv_new_header.AC.get_fir()) // FIR = 0
        {
            /*
            Send confirm if requested, accept fragment (phan chung cho ca case 10, 12)
            */
            _rcv_header.AC = _rcv_new_header.AC;
            _rcv_header.FC = _rcv_new_header.FC;
            _rcv_header.IIN = _rcv_new_header.IIN;

            // Send confirm
            if (_rcv_new_header.AC.get_con())
            {
                send_confirmed(_rcv_new_header.AC.get_seq(), false);
            }

            // accept fragment
            for (ByteVector::size_type i = 4; i < length; i++)
            {
                _asdu.push_back(buff->at(i));
            }
            _lastfrag_index = _asdu.size() - (length - 4);

            if (_rcv_new_header.AC.get_fin()) // case 12, FIN = 1, process fragment
            {
                _ap_oper_status = APOpStatusCode::Idle;
                proc_asdu();
            }
            else
            {
                _rsp_timeout.start();    // case 10, FIN = 0, start timer
            }
        }
        else
        {
            _ap_oper_status = APOpStatusCode::Idle;    // Thuc ra cho nay la thua, de vay cung khong sao
        }
    }
    else
    {
        _ap_oper_status = APOpStatusCode::Idle;    // Tat ca truong hop con lai
    }
}

void ApplicationMaster::proc_frag_uns_startup_state(ByteVector* buff, ByteVector::size_type length)
{
    /*
    Discard fragment and do not confirm
    */

    if (request_integrity_poll(false))
    {
        /*
        Initial integrity poll completed
        Co the khong thanh cong do device dang thuc hien mot lenh nao do,
        application cung cap them function de device
        thuc hien force trang thai _ap_uns_oper_status sang FirstUR
        */
        _ap_uns_oper_status = APUNSOpStatusCode::FirstUR;
    }
}

void ApplicationMaster::proc_frag_uns_first_ur_state(ByteVector* buff, ByteVector::size_type length)
{
    _rcv_header_uns.AC = _rcv_new_header.AC;
    _rcv_header_uns.FC = _rcv_new_header.FC;
    _rcv_header_uns.IIN = _rcv_new_header.IIN;

    // Send confirm if requested
    if (_rcv_new_header.AC.get_con())
    {
        send_confirmed(_rcv_new_header.AC.get_seq(), true);
    }

    // Accept fragment, process fragment
    _ap_uns_oper_status = APUNSOpStatusCode::Idle;
    _asdu_uns.clear();
    for (ByteVector::size_type i = 4; i < length; i++)
    {
        _asdu_uns.push_back(buff->at(i));
    }
    proc_asdu_uns(); // process fragment


    // Send reset IIN1.7 and perform integrity poll
    if (_rcv_new_header.IIN.device_restart())
    {
        clear_restart_bit(false);
        request_integrity_poll(false);
    }
}

void ApplicationMaster::proc_frag_uns_idle_state(ByteVector* buff, ByteVector::size_type length)
{
    bool __integrity_sent = false;

    // Send confirm if requested
    if (_rcv_new_header.AC.get_con())
    {
        send_confirmed(_rcv_new_header.AC.get_seq(), true);
    }

    if (_rcv_new_header.AC.get_seq() == _rcv_header_uns.AC.get_seq()) // received seq == recently accepted seq
    {
        /*
        Compare octet-by-octet with previous fragment
        If octets match, take no further action
        If octets do not match, accept fragment, process fragment and perform integrity poll
        */
        if (_asdu_uns.size() == (length - 4)) // match length
        {
            ByteVector::size_type i;
            for (i = 0; i < _asdu_uns.size(); i++)
            {
                if (_asdu_uns.at(i) != buff->at(i+4)) // octets do not match
                {
                    _rcv_header_uns.AC = _rcv_new_header.AC;
                    _rcv_header_uns.FC = _rcv_new_header.FC;
                    _rcv_header_uns.IIN = _rcv_new_header.IIN;

                    _asdu_uns.clear();
                    for (ByteVector::size_type j = 4; j < length; j++)
                    {
                        _asdu_uns.push_back(buff->at(j));
                    }
                    proc_asdu_uns(); // process fragment

                    // perform integrity poll
                    request_integrity_poll(false);
                    __integrity_sent = true;
                    break;
                }
            }
        }

    }
    else // received seq (N1) != recently accepted unsolicited seq (N2)
    {
        /*
        N1 == N2 + 1: accept fragment and process fragment
        N1 != N2 + 1: accept fragment, process fragment and optionally perform integrity poll
        */
        if (_rcv_new_header.AC.get_seq() == _rcv_header_uns.AC.get_next_seq()) // N1 == N2 + 1
        {
            _rcv_header_uns.AC = _rcv_new_header.AC;
            _rcv_header_uns.FC = _rcv_new_header.FC;
            _rcv_header_uns.IIN = _rcv_new_header.IIN;

            _asdu_uns.clear();
            for (ByteVector::size_type i = 4; i < length; i++)
            {
                _asdu_uns.push_back(buff->at(i));
            }
            proc_asdu_uns(); // process fragment


        }
        else // N1 != N2 + 1
        {
            _rcv_header_uns.AC = _rcv_new_header.AC;
            _rcv_header_uns.FC = _rcv_new_header.FC;
            _rcv_header_uns.IIN = _rcv_new_header.IIN;

            _asdu_uns.clear();
            for (ByteVector::size_type i = 4; i < length; i++)
            {
                _asdu_uns.push_back(buff->at(i));
            }
            proc_asdu_uns(); // process fragment

            request_integrity_poll(false);
            __integrity_sent = true;
        }
    }


    // Send reset IIN1.7 request and perform integrity poll
    if (_rcv_new_header.IIN.device_restart())
    {
        clear_restart_bit(false);
        if (!__integrity_sent) request_integrity_poll(false);
    }
}


/*********ANALYZE ASDU AND QUALIFIER TO DETACH DNP3 POINTS*********/


void ApplicationMaster::proc_asdu()
{
    _rsp_timeout.stop();
    _point_list.clear();

    ByteVector::size_type __idx = 0;
    try
    {
        int __len = _asdu.size() - 3;
        while ((int)__idx < __len)
        {
            unsigned short __grp = (unsigned short)_asdu.at(__idx++);
            unsigned short __var = (unsigned short)_asdu.at(__idx++);
            byte __qlfier = _asdu.at(__idx++);
            DataObjectCode::DataObjectCode __obj = static_cast<DataObjectCode::DataObjectCode>
                                                   (__grp << 8 | __var);
            switch (__qlfier)
            {
            case 0x00:
                proc_qualifier_0x00(__idx, __obj, _asdu);
                break;
            case 0x01:
                proc_qualifier_0x01(__idx, __obj, _asdu);
                break;
            case 0x07:
                proc_qualifier_0x07(__idx, __obj, _asdu);
                break;
            case 0x17:
                proc_qualifier_0x17(__idx, __obj, _asdu);
                break;
            case 0x28:
                proc_qualifier_0x28(__idx, __obj, _asdu);
                break;

            default:
                break;
            }
        }
    }
    catch (std::out_of_range&)
    {
        if (_debug) *_debug_stream << "[out_of_range]data object is not supported\n";
        _asdu.clear();
    }

    notify_points(false);
}

void ApplicationMaster::proc_asdu_uns()
{
    _point_list.clear();

    ByteVector::size_type __idx = 0;
    try
    {
        int __len = _asdu_uns.size() - 3;
        while ((int)__idx < __len)
        {
            unsigned short __grp = (unsigned short)_asdu_uns.at(__idx++);
            unsigned short __var = (unsigned short)_asdu_uns.at(__idx++);
            byte __qlfier = _asdu_uns.at(__idx++);
            DataObjectCode::DataObjectCode __obj = static_cast<DataObjectCode::DataObjectCode>
                                                   (__grp << 8 | __var);
            switch (__qlfier)
            {
            case 0x00:
                proc_qualifier_0x00(__idx, __obj, _asdu_uns);
                break;
            case 0x01:
                proc_qualifier_0x01(__idx, __obj, _asdu_uns);
                break;
            case 0x07:
                proc_qualifier_0x07(__idx, __obj, _asdu_uns);
                break;
            case 0x17:
                proc_qualifier_0x17(__idx, __obj, _asdu_uns);
                break;
            case 0x28:
                proc_qualifier_0x28(__idx, __obj, _asdu_uns);
                break;

            default:
                break;
            }
        }

    }
    catch (std::out_of_range&)
    {
        if (_debug) *_debug_stream << "[out_of_range]data object is not support\n";
        _asdu_uns.clear();
    }

    notify_points(true);
}

void ApplicationMaster::notify_points(bool is_uns)
{
    if (is_uns)
    {
        if (dnp3_point_uns_rcv_event)
        {
            Dnp3PointEventArg __e;
            __e.points = &_point_list;
            __e.length = _point_list.size();
            dnp3_point_uns_rcv_event->notify(this, &__e);
        }
    }
    else
    {
        if (dnp3_point_rcv_event)
        {
            Dnp3PointEventArg __e;
            __e.points = &_point_list;
            __e.length = _point_list.size();
            dnp3_point_rcv_event->notify(this, &__e);
        }
    }
}

void ApplicationMaster::proc_qualifier_0x00(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff)
{
    //Parsed message format: Range Field | DNP3 Objects...
    //Object packed without a prefix, 1-octet start, 1-octet stop

    unsigned int __start = (unsigned int)buff.at(idx++);
    unsigned int __stop = (unsigned int)buff.at(idx++);

    switch (obj)
    {
    case DataObjectCode::BI_PackedFormat :
        detach_bi_packed(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI16            :
        detach_ai16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32            :
        detach_ai32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::BI_Flags        :
        detach_bi_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::BO_Flags        :
        detach_bo_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter32       :
        detach_counter32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter16       :
        detach_counter16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter32      :
        detach_fcounter32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter16      :
        detach_fcounter16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI16_Flags      :
        detach_ai16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32_Flags      :
        detach_ai32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AO16_Flags      :
        detach_ao16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AO32_Flags      :
        detach_ao32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32FLT_Flags   :
        detach_ai32float_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI64FLT_Flags   :
        detach_ai64float_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter32_Flags :
        detach_counter32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter16_Flags :
        detach_counter16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter32_Flags:
        detach_fcounter32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter16_Flags:
        detach_fcounter16_flags(idx, __start, __stop, buff);
        break;

    default:
        buff.clear();
        break;
    }
}

void ApplicationMaster::proc_qualifier_0x01(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff)
{
    //Parsed message format: Range Field | DNP3 Objects...
    //Object packed without a prefix, 2-octet start, 2-octet stop

    unsigned short __start_lsb = (unsigned short)buff.at(idx++);
    unsigned short __start_msb = (unsigned short)buff.at(idx++);
    unsigned short __stop_lsb = (unsigned short)buff.at(idx++);
    unsigned short __stop_msb = (unsigned short)buff.at(idx++);
    unsigned int __start = (unsigned int)(__start_msb << 8 | __start_lsb);
    unsigned int __stop = (unsigned int)(__stop_msb << 8 | __stop_lsb);

    switch (obj)
    {
    case DataObjectCode::BI_PackedFormat :
        detach_bi_packed(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI16            :
        detach_ai16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32            :
        detach_ai32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::BI_Flags        :
        detach_bi_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::BO_Flags        :
        detach_bo_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter16       :
        detach_counter16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter32       :
        detach_counter32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter16      :
        detach_fcounter16(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter32      :
        detach_fcounter32(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI16_Flags      :
        detach_ai16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32_Flags      :
        detach_ai32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AO16_Flags      :
        detach_ao16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AO32_Flags      :
        detach_ao32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI32FLT_Flags   :
        detach_ai32float_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::AI64FLT_Flags   :
        detach_ai64float_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter16_Flags :
        detach_counter16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::Counter32_Flags :
        detach_counter32_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter16_Flags:
        detach_fcounter16_flags(idx, __start, __stop, buff);
        break;
    case DataObjectCode::FCounter32_Flags:
        detach_fcounter32_flags(idx, __start, __stop, buff);
        break;

    default:
        buff.clear();
        break;
    }
}

void ApplicationMaster::proc_qualifier_0x07(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff)
{
    //Parsed message format: Range Field | DNP3 Objects...
    //Object packed without a prefix, 1-octet count of objects

    unsigned int __count = (unsigned int)buff.at(idx++);

    switch (obj)
    {
    case DataObjectCode::TimeDelay_Fine             :
        detach_timedelay_fine(idx, __count, buff);
        break;
    case DataObjectCode::TimeDelay_Coarse           :
        detach_timedelay_coarse(idx, __count, buff);
        break;
    case DataObjectCode::TimeDate_CTO_AbsTime_Sync  :
        detach_timedatecto_abstime_sync(idx, __count, buff);
        break;
    case DataObjectCode::TimeDate_CTO_AbsTime_Unsync:
        detach_timedatecto_abstime_unsync(idx, __count, buff);
        break;

    default:
        buff.clear();
        break;
    }
}

void ApplicationMaster::proc_qualifier_0x17(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff)
{
    //Parsed message format: Range Field | DNP3 Objects...
    //Object prefixed with 1-octet index, 1-octet count of objects

    unsigned int __count = (unsigned int)buff.at(idx++);
    switch (obj)
    {
    case DataObjectCode::BI_Event              :
        detach_bi_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI16_Event            :
        detach_ai16_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI32_Event            :
        detach_ai32_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::CROB                  :
        detach_bocmd_echo(idx, 1, __count, buff);
        break;
    case DataObjectCode::BO_CMD_Event          :
        detach_bocmd_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI32FLT_Event         :
        detach_ai32float_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI64FLT_Event         :
        detach_ai64float_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO16                  :
        detach_ao16cmd_echo(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO16_CMD_Event        :
        detach_ao16cmd_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO32                  :
        detach_ao32cmd_echo(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO32_CMD_Event        :
        detach_ao32cmd_event(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI16_Event_Time       :
        detach_ai16_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI32_Event_Time       :
        detach_ai32_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::BI_Event_AbsTime      :
        detach_bi_event_abstime(idx, 1, __count, buff);
        break;
    case DataObjectCode::BI_Event_RltTime      :
        detach_bi_event_rltvtime(idx, 1, __count, buff);
        break;
    case DataObjectCode::BO_CMD_Event_Time     :
        detach_bocmd_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI32FLT_Event_Time    :
        detach_ai32float_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::AI64FLT_Event_Time    :
        detach_ai64float_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO16_CMD_Event_Time   :
        detach_ao16cmd_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::AO32_CMD_Event_Time   :
        detach_ao32cmd_event_time(idx, 1, __count, buff);
        break;
    case DataObjectCode::Counter16_Event_Flags :
        detach_counter16_event_flags(idx, 1, __count, buff);
        break;
    case DataObjectCode::Counter32_Event_Flags :
        detach_counter32_event_flags(idx, 1, __count, buff);
        break;
    case DataObjectCode::FCounter16_Event_Flags:
        detach_fcounter16_event_flags(idx, 1, __count, buff);
        break;
    case DataObjectCode::FCounter32_Event_Flags:
        detach_fcounter32_event_flags(idx, 1, __count, buff);
        break;

    default:
        buff.clear();
        break;
    }
}

void ApplicationMaster::proc_qualifier_0x28(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff)
{
    //Parsed message format: Range Field | DNP3 Objects...
    //Object prefixed with 2-octet index, 2-octet count of objects

    unsigned short __count_lsb = (unsigned short)buff.at(idx++);
    unsigned short __count_msb = (unsigned short)buff.at(idx++);
    unsigned int __count = (unsigned int)(__count_msb << 8 | __count_lsb);
    switch (obj)
    {
    case DataObjectCode::BI_Event              :
        detach_bi_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI16_Event            :
        detach_ai16_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI32_Event            :
        detach_ai32_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::CROB                  :
        detach_bocmd_echo(idx, 2, __count, buff);
        break;
    case DataObjectCode::BO_CMD_Event          :
        detach_bocmd_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI32FLT_Event         :
        detach_ai32float_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI64FLT_Event         :
        detach_ai64float_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO16                  :
        detach_ao16cmd_echo(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO16_CMD_Event        :
        detach_ao16cmd_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO32                  :
        detach_ao32cmd_echo(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO32_CMD_Event        :
        detach_ao32cmd_event(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI16_Event_Time       :
        detach_ai16_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI32_Event_Time       :
        detach_ai32_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::BI_Event_AbsTime      :
        detach_bi_event_abstime(idx, 2, __count, buff);
        break;
    case DataObjectCode::BI_Event_RltTime      :
        detach_bi_event_rltvtime(idx, 2, __count, buff);
        break;
    case DataObjectCode::BO_CMD_Event_Time     :
        detach_bocmd_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI32FLT_Event_Time    :
        detach_ai32float_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::AI64FLT_Event_Time    :
        detach_ai64float_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO16_CMD_Event_Time   :
        detach_ao16cmd_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::AO32_CMD_Event_Time   :
        detach_ao32cmd_event_time(idx, 2, __count, buff);
        break;
    case DataObjectCode::Counter16_Event_Flags :
        detach_counter16_event_flags(idx, 2, __count, buff);
        break;
    case DataObjectCode::Counter32_Event_Flags :
        detach_counter32_event_flags(idx, 2, __count, buff);
        break;
    case DataObjectCode::FCounter16_Event_Flags:
        detach_fcounter16_event_flags(idx, 2, __count, buff);
        break;
    case DataObjectCode::FCounter32_Event_Flags:
        detach_fcounter32_event_flags(idx, 2, __count, buff);
        break;

    default:
        buff.clear();
        break;
    }
}


/***********DATA OBJECT PROCESSION***********/

void ApplicationMaster::detach_bi_packed(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;
    unsigned int __num_of_bytes = __num_of_points / 8;
    unsigned int __remain_points = __num_of_points % 8;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::si;
    __point.flags = static_cast<byte>(SingleInputFlagCode::ONLINE);
    byte __byt = 0x00;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_bytes; i++)
    {
        __byt = buff.at(idx++);

        __val = (__byt >> 0) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 1) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 2) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 3) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 4) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 5) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 6) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
        __val = (__byt >> 7) & 0x01;
        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }

    if (__remain_points != 0)
    {
        unsigned int __i = 0;
        __byt = buff.at(idx++);
        while (__remain_points > 0)
        {
            __val = (__byt >> __i) & 0x01;
            __point.index = __point_idx++;
            __point.value = num_to_str(__val);
            _point_list.push_back(__point);
            __remain_points--;
            __i++;
        }
    }
}

void ApplicationMaster::detach_bi_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::si;
    byte __byt = 0x00;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __byt = buff.at(idx++);
        __val = (__byt >> 7) & 0x01;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        __point.flags = __byt;
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bo_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::bo;
    byte __byt = 0x00;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __byt = buff.at(idx++);
        __val = (__byt >> 7) & 0x01;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        __point.flags = __byt;
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter32;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __val |= (unsigned short)buff.at(idx++) << 16;
        __val |= (unsigned short)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter16;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter32;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __val |= (unsigned short)buff.at(idx++) << 16;
        __val |= (unsigned short)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter16;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter32;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __val |= (unsigned short)buff.at(idx++) << 16;
        __val |= (unsigned short)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter16;
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter32;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __val |= (unsigned short)buff.at(idx++) << 16;
        __val |= (unsigned short)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter16;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    unsigned short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32;
    int __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai16;
    short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    int __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai16;
    __point.flags = static_cast<byte>(CounterFlagCode::ONLINE);
    short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32float_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32float;
    byte arr[4];

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);

        __point.index = __point_idx++;
        __point.value = num_to_str(bytes_to_float(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai64float_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai64float;
    byte arr[8];

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        arr[7] = buff.at(idx++);
        arr[6] = buff.at(idx++);
        arr[5] = buff.at(idx++);
        arr[4] = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);

        __point.index = __point_idx++;
        __point.value = num_to_str(bytes_to_double(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ao16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao16;
    short __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ao32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff)
{
    unsigned int __point_idx = start;
    unsigned int __num_of_points = stop - start + 1;

    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao32;
    int __val = 0;

    for (unsigned int i = 0; i < __num_of_points; i++)
    {
        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;

        __point.index = __point_idx++;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bi_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::si;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __point.value = num_to_str(__point.flags >> 7 & 0x01);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bi_event_abstime(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::si;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __point.value = num_to_str(__point.flags >> 7 & 0x01);
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bi_event_rltvtime(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::si;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __point.value = num_to_str(__point.flags >> 7 & 0x01);
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp += _time_cto;
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter32_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter32;
    unsigned int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (unsigned int)buff.at(idx++);
        __val |= (unsigned int)buff.at(idx++) << 8;
        __val |= (unsigned int)buff.at(idx++) << 16;
        __val |= (unsigned int)buff.at(idx++) << 24;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_counter16_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::counter16;
    unsigned short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter32_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter32;
    unsigned int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (unsigned int)buff.at(idx++);
        __val |= (unsigned int)buff.at(idx++) << 8;
        __val |= (unsigned int)buff.at(idx++) << 16;
        __val |= (unsigned int)buff.at(idx++) << 24;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_fcounter16_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::fcounter16;
    unsigned short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (unsigned short)buff.at(idx++);
        __val |= (unsigned short)buff.at(idx++) << 8;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32;
    int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai16_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai16;
    short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32;
    int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai16_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai16;
    short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(__val);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32float_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32float;
    byte arr[4];
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);
        __point.value = num_to_str(bytes_to_float(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai64float_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai64float;
    byte arr[8];
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        arr[7] = buff.at(idx++);
        arr[6] = buff.at(idx++);
        arr[5] = buff.at(idx++);
        arr[4] = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);
        __point.value = num_to_str(bytes_to_double(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai32float_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai32float;
    byte arr[4];
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(bytes_to_float(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ai64float_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ai64float;
    byte arr[8];
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        arr[7] = buff.at(idx++);
        arr[6] = buff.at(idx++);
        arr[5] = buff.at(idx++);
        arr[4] = buff.at(idx++);
        arr[3] = buff.at(idx++);
        arr[2] = buff.at(idx++);
        arr[1] = buff.at(idx++);
        arr[0] = buff.at(idx++);
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(bytes_to_double(arr));
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bocmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    Dnp3PointVector __cmd_point_list;
    __point.type_id = PointTypeCode::bocmd;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        byte __ctrl_code = buff.at(idx++);
        __point.TCC = (CmdTrClsCode::CmdTrClsCode)((__ctrl_code >> 6) & 0x03);
        __point.op_type = (CmdOperTypeCode::CmdOperTypeCode)(__ctrl_code & 0x0f);
        __point.cmd_count = buff.at(idx++);

        __point.cmd_ontime = (unsigned int)buff.at(idx++);
        __point.cmd_ontime |= (unsigned int)buff.at(idx++) << 8;
        __point.cmd_ontime |= (unsigned int)buff.at(idx++) << 16;
        __point.cmd_ontime |= (unsigned int)buff.at(idx++) << 24;

        __point.cmd_offtime = (unsigned int)buff.at(idx++);
        __point.cmd_offtime |= (unsigned int)buff.at(idx++) << 8;
        __point.cmd_offtime |= (unsigned int)buff.at(idx++) << 16;
        __point.cmd_offtime |= (unsigned int)buff.at(idx++) << 24;

        __point.flags = buff.at(idx++);
        __point.value = num_to_str((__point.flags >> 7) & 0x01);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        __cmd_point_list.push_back(__point);
    }

    if (dnp3_cmd_echo_event)
    {
        Dnp3PointEventArg __e;
        __e.points = &__cmd_point_list;
        __e.length = __cmd_point_list.size();
        dnp3_cmd_echo_event->notify(this, &__e);
    }
}

void ApplicationMaster::detach_bocmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::bocmdstt;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __point.value = num_to_str(__point.flags >> 7 & 0x01);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_bocmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::bocmdstt;

    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(__point.flags >> 7 & 0x01);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }

}

void ApplicationMaster::detach_ao16cmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    Dnp3PointVector __cmd_point_list;
    __point.type_id = PointTypeCode::ao16cmd;
    short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;
        __point.value = num_to_str(__val);
        __point.flags = buff.at(idx++);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags);
        __cmd_point_list.push_back(__point);
    }

    if (dnp3_cmd_echo_event)
    {
        Dnp3PointEventArg __e;
        __e.points = &__cmd_point_list;
        __e.length = __cmd_point_list.size();
        dnp3_cmd_echo_event->notify(this, &__e);
    }
}

void ApplicationMaster::detach_ao16cmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao16cmdstt;
    short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;
        __point.value = num_to_str(__val);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }

}

void ApplicationMaster::detach_ao16cmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao16cmdstt;
    short __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (short)buff.at(idx++);
        __val |= (short)buff.at(idx++) << 8;
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(__val);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ao32cmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    Dnp3PointVector __cmd_point_list;
    __point.type_id = PointTypeCode::ao32cmd;
    int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;
        __point.value = num_to_str(__val);
        __point.flags = buff.at(idx++);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags);
        __cmd_point_list.push_back(__point);
    }

    if (dnp3_cmd_echo_event)
    {
        Dnp3PointEventArg __e;
        __e.points = &__cmd_point_list;
        __e.length = __cmd_point_list.size();
        dnp3_cmd_echo_event->notify(this, &__e);
    }
}

void ApplicationMaster::detach_ao32cmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao32cmdstt;
    int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;
        __point.value = num_to_str(__val);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_ao32cmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff)
{
    Dnp3Point __point;
    __point.type_id = PointTypeCode::ao32cmdstt;
    int __val = 0;
    for (unsigned int i = 0; i < count; i++)
    {
        __point.index = (unsigned int)buff.at(idx++);
        if (prefixSize == 2)
        {
            __point.index |= (unsigned int)buff.at(idx++) << 8;
        }

        __point.flags = buff.at(idx++);
        __val = (int)buff.at(idx++);
        __val |= (int)buff.at(idx++) << 8;
        __val |= (int)buff.at(idx++) << 16;
        __val |= (int)buff.at(idx++) << 24;
        __point.timestamp = (unsigned long long)buff.at(idx++);
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 8;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 16;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 24;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 32;
        __point.timestamp |= (unsigned long long)buff.at(idx++) << 40;
        __point.value = num_to_str(__val);
        __point.cmd_status = (CmdStatusCode::CmdStatusCode)(__point.flags & 0x7f);
        _point_list.push_back(__point);
    }
}

void ApplicationMaster::detach_timedatecto_abstime_sync(ByteVector::size_type& idx, unsigned int count, ByteVector& buff)
{
    for (unsigned int i = 0; i < count; i++)
    {
        _time_cto = (unsigned long long)buff.at(idx++);
        _time_cto |= (unsigned long long)buff.at(idx++) << 8;
        _time_cto |= (unsigned long long)buff.at(idx++) << 16;
        _time_cto |= (unsigned long long)buff.at(idx++) << 24;
        _time_cto |= (unsigned long long)buff.at(idx++) << 32;
        _time_cto |= (unsigned long long)buff.at(idx++) << 40;
    }
}

void ApplicationMaster::detach_timedatecto_abstime_unsync(ByteVector::size_type& idx, unsigned int count, ByteVector& buff)
{
    for (unsigned int i = 0; i < count; i++)
    {
        _time_cto = (unsigned long long)buff.at(idx++);
        _time_cto |= (unsigned long long)buff.at(idx++) << 8;
        _time_cto |= (unsigned long long)buff.at(idx++) << 16;
        _time_cto |= (unsigned long long)buff.at(idx++) << 24;
        _time_cto |= (unsigned long long)buff.at(idx++) << 32;
        _time_cto |= (unsigned long long)buff.at(idx++) << 40;
    }
}

void ApplicationMaster::detach_timedelay_coarse(ByteVector::size_type& idx, unsigned int count, ByteVector& buff)
{
    //Resolution here is 1 second, so must multi by 100
    for (unsigned int i = 0; i < count; i++)
    {
        _time_delay = (unsigned long long)buff.at(idx++);
        _time_delay |= (unsigned long long)buff.at(idx++) << 8;
        _time_delay *= 1000;
    }

    if (dnp3_time_delay_event)
    {
        dnp3_time_delay_event->notify(this, NULL);
    }
}

void ApplicationMaster::detach_timedelay_fine(ByteVector::size_type& idx, unsigned int count, ByteVector& buff)
{
    //Resolution here is 1 millisecond
    for (unsigned int i = 0; i < count; i++)
    {
        _time_delay = (unsigned long long)buff.at(idx++);
        _time_delay |= (unsigned long long)buff.at(idx++) << 8;
    }

    if (dnp3_time_delay_event)
    {
        dnp3_time_delay_event->notify(this, NULL);
    }
}



/***********LIST OF SERVICES PROVIDED BY APPLICATION LAYER***********/


bool ApplicationMaster::read_class_data(int isClass, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    if (_ap_oper_status != APOpStatusCode::Idle) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::READ;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;

    switch (isClass)
    {
    case 0:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x01;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 1:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 2:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x03;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 3:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x04;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 123:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment[5] = 0x3c;
        _send_fragment[6] = 0x03;
        _send_fragment[7] = 0x06;
        _send_fragment[8] = 0x3c;
        _send_fragment[9] = 0x04;
        _send_fragment[10] = 0x06;
        _send_fragment_len = 11;
        break;

    default:
        return false;
    }

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);
    _ap_oper_status = APOpStatusCode::AwaitFirst;
    _rsp_timeout.start();

    return true;
}

bool ApplicationMaster::read_class_data(int isClass, unsigned int count, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    if (_ap_oper_status != APOpStatusCode::Idle) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::READ;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x3c;

    switch (isClass)
    {
    case 0:
        _send_fragment[3] = 0x01;
        break;
    case 1:
        _send_fragment[3] = 0x02;
        break;
    case 2:
        _send_fragment[3] = 0x03;
        break;
    case 3:
        _send_fragment[3] = 0x04;
        break;

    default:
        return false;
    }

    if (count <= 255)
    {
        _send_fragment[4] = 0x07;
        _send_fragment[5] = (byte)count;
        _send_fragment_len = 6;
    }
    else if ((255u < count) && (count <= 65535u))
    {
        _send_fragment[4] = 0x08;
        _send_fragment[5] = Dnp3Master::get_low_byte_uint16(count);
        _send_fragment[6] = Dnp3Master::get_high_byte_uint16(count);
        _send_fragment_len = 7;
    }
    else if ((65535u < count) && (count <= 4294967295u))
    {
        _send_fragment[4] = 0x09;
        _send_fragment[5] = Dnp3Master::get_byte_int32(count, 1);
        _send_fragment[6] = Dnp3Master::get_byte_int32(count, 2);
        _send_fragment[7] = Dnp3Master::get_byte_int32(count, 3);
        _send_fragment[8] = Dnp3Master::get_byte_int32(count, 4);
        _send_fragment_len = 9;
    }
    else
    {
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
    }

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);
    _ap_oper_status = APOpStatusCode::AwaitFirst;
    _rsp_timeout.start();

    return true;
}

bool ApplicationMaster::send_confirmed(int seq, bool isUNS)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(isUNS);
    _send_header.AC.set_seq((byte)seq);
    _send_header.FC = APFunctionCode::CONFRIM;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment_len = 2;

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, false);

    return true;
}

bool ApplicationMaster::request_integrity_poll(bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    if (_ap_oper_status != APOpStatusCode::Idle) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::READ;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;

    _send_fragment[2]  = 0x3c;
    _send_fragment[3]  = 0x02;
    _send_fragment[4]  = 0x06;
    _send_fragment[5]  = 0x3c;
    _send_fragment[6]  = 0x03;
    _send_fragment[7]  = 0x06;
    _send_fragment[8]  = 0x3c;
    _send_fragment[9]  = 0x04;
    _send_fragment[10] = 0x06;
    _send_fragment[11] = 0x3c;
    _send_fragment[12] = 0x01;
    _send_fragment[13] = 0x06;
    _send_fragment_len = 14;

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);
    _ap_oper_status = APOpStatusCode::AwaitFirst;
    _rsp_timeout.start();

    if (_debug) send_debug();

    return true;
}

bool ApplicationMaster::clear_restart_bit(bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::WRITE;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x50; // Group 80
    _send_fragment[3] = 0x01; // Variant 01
    _send_fragment[4] = 0x00; // Qualifier 00
    _send_fragment[5] = 0x07; // Start Idx (IIN1.7 DEVICE RESTART)
    _send_fragment[6] = 0x07; // Stop Idx
    _send_fragment[7] = 0x00; // Packed Object
    _send_fragment_len = 8;
    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (_debug) send_debug();

    return true;
}

bool ApplicationMaster::request_cold_restart(bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::COLD_RESTART;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment_len = 2;
    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (_debug) send_debug();

    return true;
}

bool ApplicationMaster::delay_measurement(bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::DELAY_MEASURE;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment_len = 2;

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }
    return true;
}

bool ApplicationMaster::record_current_time(bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::RECORD_CURRENT_TIME;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment_len = 2;

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }
    return true;
}

bool ApplicationMaster::write_abstime(unsigned long long value, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::WRITE;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x32; // Grp
    _send_fragment[3] = 0x01; // Var
    _send_fragment[4] = 0x07; // Qualifier
    _send_fragment[5] = 0x01; // Range
    _send_fragment[6] = (byte)(value & 0xff);
    _send_fragment[7] = (byte)(value >> 8 & 0xff);
    _send_fragment[8] = (byte)(value >> 16 & 0xff);
    _send_fragment[9] = (byte)(value >> 24 & 0xff);
    _send_fragment[10] = (byte)(value >> 32 & 0xff);
    _send_fragment[11] = (byte)(value >> 40 & 0xff);
    _send_fragment_len = 12;

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }
    return true;
}

bool ApplicationMaster::write_last_recorded_time(unsigned long long value, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::WRITE;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x32; // Grp
    _send_fragment[3] = 0x03; // Var
    _send_fragment[4] = 0x07; // Qualifier
    _send_fragment[5] = 0x01; // Range
    _send_fragment[6] = (byte)(value & 0xff);
    _send_fragment[7] = (byte)(value >> 8 & 0xff);
    _send_fragment[8] = (byte)(value >> 16 & 0xff);
    _send_fragment[9] = (byte)(value >> 24 & 0xff);
    _send_fragment[10] = (byte)(value >> 32 & 0xff);
    _send_fragment[11] = (byte)(value >> 40 & 0xff);
    _send_fragment_len = 12;
    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (_debug) send_debug();

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }
    return true;
}

bool ApplicationMaster::read_time_and_date(bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::READ;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x32; // Grp
    _send_fragment[3] = 0x01; // Var
    _send_fragment[4] = 0x07; // Qualifier
    _send_fragment[5] = 0x01; // Range
    _send_fragment_len = 6;
    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (_debug) send_debug();

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }
    return true;
}

bool ApplicationMaster::set_enable_uns(int isClass, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    if (_ap_oper_status != APOpStatusCode::Idle) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::ENABLE_UNSOLICITED;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;

    switch (isClass)
    {
    case 1:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 2:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x03;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 3:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x04;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 123:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment[5] = 0x3c;
        _send_fragment[6] = 0x03;
        _send_fragment[7] = 0x06;
        _send_fragment[8] = 0x3c;
        _send_fragment[9] = 0x04;
        _send_fragment[10] = 0x06;
        _send_fragment_len = 11;
        break;

    default:
        return false;
    }

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);
    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }

    return true;
}

bool ApplicationMaster::set_disable_uns(int isClass, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */
    if (_ap_oper_status != APOpStatusCode::Idle) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());
    _send_header.FC = APFunctionCode::DISABLE_UNSOLICITED;

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;

    switch (isClass)
    {
    case 1:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 2:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x03;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 3:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x04;
        _send_fragment[4] = 0x06;
        _send_fragment_len = 5;
        break;
    case 123:
        _send_fragment[2] = 0x3c;
        _send_fragment[3] = 0x02;
        _send_fragment[4] = 0x06;
        _send_fragment[5] = 0x3c;
        _send_fragment[6] = 0x03;
        _send_fragment[7] = 0x06;
        _send_fragment[8] = 0x3c;
        _send_fragment[9] = 0x04;
        _send_fragment[10] = 0x06;
        _send_fragment_len = 11;
        break;

    default:
        return false;
    }

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);
    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }

    return true;
}

bool ApplicationMaster::send_cmd_crob(Dnp3Point* point, int cmdType, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */
    if (_ap_oper_status != APOpStatusCode::Idle) return false;
    if (point->type_id != PointTypeCode::bocmd) return false;

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());

    switch (cmdType)
    {
    case 1:
        _send_header.FC = APFunctionCode::SELECT;
        break;
    case 2:
        _send_header.FC = APFunctionCode::OPERATE;
        break;
    case 3:
        _send_header.FC = APFunctionCode::DIRECT_OPERATE;
        break;
    case 4:
        _send_header.FC = APFunctionCode::DIRECT_OPERATE_NR;
        break;

    default:
        return false;
    }

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;
    _send_fragment[2] = 0x0c; // Group
    _send_fragment[3] = 0x01; // Variant

    if (point->index <= 255)
    {
        _send_fragment[4] = 0x17; // Qualifier
        _send_fragment[5] = 0x01; // Range
        _send_fragment[6] = (byte)(point->index);
        _send_fragment_len = 7;
    }
    else
    {
        _send_fragment[4] = 0x28;
        _send_fragment[5] = Dnp3Master::get_low_byte_uint16(1); // Range LSB
        _send_fragment[6] = Dnp3Master::get_high_byte_uint16(1); // Range MSB
        _send_fragment[7] = Dnp3Master::get_low_byte_uint16(point->index); // Index LSB
        _send_fragment[8] = Dnp3Master::get_high_byte_uint16(point->index); // Index MSB
        _send_fragment_len = 9;
    }

    byte ctrlCode = (static_cast<byte>(point->TCC) << 6) | (static_cast<byte>(point->op_type) & 0x0f);
    if (point->clear_bit)
    {
        ctrlCode |= 0x20;
    }

    _send_fragment[_send_fragment_len++] = ctrlCode;
    _send_fragment[_send_fragment_len++] = point->cmd_count;

    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_ontime, 1);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_ontime, 2);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_ontime, 3);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_ontime, 4);

    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_offtime, 1);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_offtime, 2);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_offtime, 3);
    _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(point->cmd_offtime, 4);

    _send_fragment[_send_fragment_len++] = 0x00; // Control Status Code (always = 0 in request)

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }

    return true;
}

bool ApplicationMaster::send_cmd_ao(Dnp3Point* point, int cmdType, bool isWait, bool isConfirm)
{
    /* Fragment: AC|FC|ObjGrp|ObjVariant|Qualifier */

    if (_ap_oper_status != APOpStatusCode::Idle) return false;
    switch (point->type_id)
    {
    case PointTypeCode::ao16cmd:
        _send_fragment[2] = 0x29;
        _send_fragment[3] = 0x02;
        break;
    case PointTypeCode::ao32cmd:
        _send_fragment[2] = 0x29;
        _send_fragment[3] = 0x01;
        break;

    default:
        return false;
    }

    _send_header.AC.set_fir(true);
    _send_header.AC.set_fin(true);
    _send_header.AC.set_con(false);
    _send_header.AC.set_uns(false);
    _send_header.AC.set_seq(_send_header.AC.get_next_seq());

    switch (cmdType)
    {
    case 1:
        _send_header.FC = APFunctionCode::SELECT;
        break;
    case 2:
        _send_header.FC = APFunctionCode::OPERATE;
        break;
    case 3:
        _send_header.FC = APFunctionCode::DIRECT_OPERATE;
        break;
    case 4:
        _send_header.FC = APFunctionCode::DIRECT_OPERATE_NR;
        break;

    default:
        return false;
    }

    _send_fragment_len = 0;
    _send_fragment[0] = _send_header.AC.get_byte();
    _send_fragment[1] = (byte)_send_header.FC;

    if (point->index <= 255)
    {
        _send_fragment[4] = 0x17; // Qualifier
        _send_fragment[5] = 0x01; // Range
        _send_fragment[6] = (byte)(point->index);
        _send_fragment_len = 7;
    }
    else
    {
        _send_fragment[4] = 0x28;
        _send_fragment[5] = Dnp3Master::get_low_byte_uint16(1); // Range LSB
        _send_fragment[6] = Dnp3Master::get_high_byte_uint16(1); // Range MSB
        _send_fragment[7] = Dnp3Master::get_low_byte_uint16(point->index); // Index LSB
        _send_fragment[8] = Dnp3Master::get_high_byte_uint16(point->index); // Index MSB
        _send_fragment_len = 9;
    }

    switch (point->type_id)
    {
    case PointTypeCode::ao16cmd:
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_low_byte_uint16(str_to_num<short>(point->value));
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_high_byte_uint16(str_to_num<short>(point->value));
        break;

    case PointTypeCode::ao32cmd:
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(str_to_num<int>(point->value), 1);
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(str_to_num<int>(point->value), 2);
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(str_to_num<int>(point->value), 3);
        _send_fragment[_send_fragment_len++] = Dnp3Master::get_byte_int32(str_to_num<int>(point->value), 4);
        break;

    default:
        break;
    }

    _send_fragment[_send_fragment_len++] = 0x00; // Control status code (always = 0 in request

    if (_debug) send_debug();

    _transport_loader->send_userdata_request(_send_fragment, _send_fragment_len, isConfirm);

    if (isWait)
    {
        _ap_oper_status = APOpStatusCode::AwaitFirst;
        _rsp_timeout.start();
    }

    return true;
}

} // Dnp3Master namespace

