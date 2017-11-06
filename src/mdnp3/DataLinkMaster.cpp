#include "mdnp3/pch.h"
#include "mdnp3/DataLinkMaster.h"

namespace Dnp3Master
{
DataLinkMaster::DataLinkMaster()
{
    // TO DO ...
    segment_received_event = NULL;
    send_frame_event = NULL;
    _sec_station_is_reset = false;
    _sec_station_is_overflow = false;
    _oper_status = LinkOpStatusCode::SecUnResetIdle;
    _retry_max = 3;
    _retry_count = 0;
    _NFCB = 0;
    _src_address = 0;
    _dest_address = 0;
    _send_frame_len = 0;
    _rcv_frame_len = 0;
    _send_userdata_len = 0;
    _debug = false;
    _debug_stream = NULL;
}


DataLinkMaster::~DataLinkMaster()
{
    // TO DO ...
}


bool DataLinkMaster::send_unconfirmed_userdata(byte usdata[], int length)
{
    bool __r = false;
    switch (_oper_status)
    {
    case LinkOpStatusCode::SecUnResetIdle:
    case LinkOpStatusCode::SecResetIdle:
        for (int i = 0; i < length; i++)
        {
            _send_userdata[i] = usdata[i];
        }
        _send_userdata_len = length;
        unconfirmed_userdata();
        __r = true;
        break;
    default:
        break;

    }

    return __r;
}


bool DataLinkMaster::send_confirmed_userdata(byte usdata[], int length)
{
    bool __r = false;
    switch (_oper_status)
    {
    case LinkOpStatusCode::SecResetIdle:
    {
        for (int i = 0; i < length; i++)
        {
            _send_userdata[i] = usdata[i];
        }
        _send_userdata_len = length;
        confirmed_userdata();
        _retry_count = 0;
        _rsp_timer.start();
        _oper_status = LinkOpStatusCode::CfmDataWait;
        __r = true;
    }
    break;
    case LinkOpStatusCode::SecUnResetIdle:
    {
        for (int i = 0; i < length; i++)
        {
            _send_userdata[i] = usdata[i];
        }
        _send_userdata_len = length;
        confirmed_userdata();
        _retry_count = 0;
        _rsp_timer.start();
        _oper_status = LinkOpStatusCode::ResetLinkWait_2;
        __r = true;
    }
    break;
    default:
        break;
    }

    return __r;
}


bool DataLinkMaster::send_test_link_state()
{
    if (_oper_status == LinkOpStatusCode::SecResetIdle)
    {
        test_link_state();
        _oper_status = LinkOpStatusCode::TestWait;
        _retry_count = 0;
        _rsp_timer.start();
        return true;
    }

    return false;
}


void DataLinkMaster::act_on_timeout()
{
    if (_retry_count >= _retry_max)
    {
        reset_layer_state();
    }

    bool __retry = true;
    switch (_oper_status)
    {
    case LinkOpStatusCode::ResetLinkWait_1:
        reset_link_state();
        break;
    case LinkOpStatusCode::ResetLinkWait_2:
        reset_link_state();
        break;
    case LinkOpStatusCode::UR_LinkStatusWait:
        request_link_status();
        break;
    case LinkOpStatusCode::CfmDataWait:
        confirmed_userdata();
        break;
    case LinkOpStatusCode::R_LinkStatusWait:
        request_link_status();
        break;
    case LinkOpStatusCode::TestWait:
        test_link_state();
        break;
    default:
        __retry = false;
        _rsp_timer.stop();
        break;
    }
    if (__retry)
    {
        _retry_count++;
        _rsp_timer.start();
    }
}

void DataLinkMaster::reset_layer_state()
{
    _oper_status = LinkOpStatusCode::SecUnResetIdle;
    _sec_station_is_reset = false;
    _sec_station_is_overflow = false;
    _rsp_timer.stop();
    _retry_count = 0;
}

/*************** PROCESS FRAME RECEIVED FROM OUTSTATION ******************/

void DataLinkMaster::on_frame_received(void* sender, FrameDataEventArg* e)
{
    if (e->length <= 0) return;

#ifdef LINUX_ONLY
    timeb __utc_now;
    ftime(&__utc_now);
    _last_rcv_time = (long long int)(__utc_now.time)*1000 + __utc_now.millitm;
#else
    milliseconds __utc_now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
    _last_rcv_time = __utc_now.count();
#endif // LINUX_ONLY

    byte* __buff = e->frame;
    int __len = e->length;

    for (int i = 0; i < __len; i++)
    {
        _rcv_frame[i] = __buff[i];
    }
    _rcv_frame_len = __len;
    _rcv_header.set_header(_rcv_frame);

    /* Check debug */
    if (_debug)
    {
        *_debug_stream << "[DataLink]<++++";
        *_debug_stream << std::hex << std::setfill('0');
        for (int i = 0; i < _rcv_frame_len; i++)
            *_debug_stream << " " << std::setw(2) << (int)_rcv_frame[i];
        DataLinkControlField __ctrl = _rcv_header.get_control();
        *_debug_stream
                << std::dec     << "\nDataLink header response:\n"
                << "SRC: "      << _rcv_header.get_source()
                << ", DEST: "   << _rcv_header.get_dest()
                << ", LEN: "    << _rcv_header.get_length()
                << ", PRM: "    << __ctrl.get_prm()
                << ", DIR: "    << __ctrl.get_dir()
                << ", FCB: "    << __ctrl.get_fcb()
                << ", DFC: "    << __ctrl.get_fcv_dfc()
                << ", FC: "     << FunctionCode::to_string(__ctrl.get_fc())
                << "\n";
    }

    if (_rcv_header.get_control().get_prm()) //PRM = 1, Primary to Secondary
    {
        FunctionCode::FunctionCode __fc = _rcv_header.get_control().get_fc();
        if (__fc == FunctionCode::USERDATA_CONFIRM)
        {
            ack();
            proc_frame_userdata();
        }
        else if (__fc == FunctionCode::USERDATA_NO_CONFIRM)
        {
            proc_frame_userdata();
        }
        else
        {
            ack();
        }
    }
    else //PRM = 0, Secondary to Primary
    {
        _rsp_timer.stop();

        switch (_oper_status)
        {
        case LinkOpStatusCode::ResetLinkWait_1:
            proc_frame_resetlink_wait1();
            break;
        case LinkOpStatusCode::ResetLinkWait_2:
            proc_frame_resetlink_wait2();
            break;
        case LinkOpStatusCode::UR_LinkStatusWait:
            proc_frame_UR_linkstatus_wait();
            break;
        case LinkOpStatusCode::R_LinkStatusWait:
            proc_frame_R_linkstatus_wait();
            break;
        case LinkOpStatusCode::TestWait:
            proc_frame_test_wait();
            break;
        case LinkOpStatusCode::CfmDataWait:
            proc_frame_cfmdata_wait();
            break;
        default:
            break;
        }
    }
}


void DataLinkMaster::proc_frame_userdata()
{
    pop_userdata_from_frame();
    notify_userdata();
}


void DataLinkMaster::proc_frame_sec_unreset_idle()
{
    // Do nothing
}


void DataLinkMaster::proc_frame_sec_reset_idle()
{
    // Do nothing
}


void DataLinkMaster::proc_frame_resetlink_wait1()
{
    if (_rcv_header.get_control().get_fc() == FunctionCode::ACK)
    {
        _NFCB = 1;
        _sec_station_is_reset = true;
        _oper_status = LinkOpStatusCode::SecResetIdle;
    }
    else
    {
        _sec_station_is_reset = false;
        _oper_status = LinkOpStatusCode::SecUnResetIdle;
    }
}


void DataLinkMaster::proc_frame_resetlink_wait2()
{
    if (_rcv_header.get_control().get_fc() == FunctionCode::ACK)
    {
        _NFCB = 1;
        _sec_station_is_reset = true;
        confirmed_userdata();
        _oper_status = LinkOpStatusCode::CfmDataWait;
    }
    else
    {
        _sec_station_is_reset = false;
        _oper_status = LinkOpStatusCode::SecUnResetIdle;
    }
}


void DataLinkMaster::proc_frame_UR_linkstatus_wait()
{
    _oper_status = LinkOpStatusCode::SecUnResetIdle;
    if (_rcv_header.get_control().get_fc() == FunctionCode::LINK_STATUS)
    {
        if (_rcv_header.get_control().get_fcv_dfc())
        {
            _sec_station_is_overflow = true;
        }
        else _sec_station_is_overflow = false;
    }
}


void DataLinkMaster::proc_frame_test_wait()
{
    if (_rcv_header.get_control().get_fc() == FunctionCode::ACK)
    {
        _NFCB = (_NFCB ^ 0xff) & 0x01; //Toggle _NFCB
        _oper_status = LinkOpStatusCode::SecResetIdle;

    }
    else
    {
        _sec_station_is_reset = false;
        _oper_status = LinkOpStatusCode::SecUnResetIdle;
    }
}


void DataLinkMaster::proc_frame_cfmdata_wait()
{
    FunctionCode::FunctionCode __fc = _rcv_header.get_control().get_fc();
    if ( __fc == FunctionCode::ACK)
    {
        _NFCB = (_NFCB ^ 0xff) & 0x01; //Toggle _NFCB
        _oper_status = LinkOpStatusCode::SecResetIdle;
    }
    else if (__fc == FunctionCode::NACK)
    {
        if (_rcv_header.get_control().get_fcv_dfc())
        {
            _sec_station_is_reset = false;
            _sec_station_is_overflow = true;
            _oper_status = LinkOpStatusCode::SecUnResetIdle;
        }
        else
        {
            _retry_count = 0;
            _sec_station_is_reset = false;
            reset_link_state();
            _oper_status = LinkOpStatusCode::ResetLinkWait_2;
        }
    }
    else
    {
        _sec_station_is_reset = false;
        _oper_status = LinkOpStatusCode::SecUnResetIdle;
    }
}


void DataLinkMaster::proc_frame_R_linkstatus_wait()
{
    _oper_status = LinkOpStatusCode::SecResetIdle;
    if (_rcv_header.get_control().get_fc() == FunctionCode::LINK_STATUS)
    {
        if (_rcv_header.get_control().get_fcv_dfc())
        {
            _sec_station_is_overflow = true;
        }
        else _sec_station_is_overflow = false;
    }
}


/************************** DEFINE REQUEST *******************************/


void DataLinkMaster::reset_link_state()
{
    //DIR = 1; PRM = 1; FCB = 0; FCV = 0; FC = 0;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = 0x05;
    _send_frame[3] = 0xC0;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;

    if (_debug) *_debug_stream << "[DataLink]Reset Link state\n";

    send_frame(); //Send frame
}


void DataLinkMaster::request_link_status()
{
    //DIR = 1; PRM = 1; FCB = 0; FCV = 0; FC = 9;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = 0x05;
    _send_frame[3] = 0xC9;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;

    if (_debug) *_debug_stream << "[DataLink]Request link status\n";

    send_frame(); //Send frame
}


void DataLinkMaster::test_link_state()
{
    //DIR = 1; PRM = 1; FCB = ?; FCV = 1; FC = 2;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = 0x05;
    _send_frame[3] = 0xd0 | (_NFCB << 5 & 0x20) | 0x02;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;

    if (_debug) *_debug_stream << "[DataLink]Test link\n";

    send_frame(); //Send frame
}


void DataLinkMaster::confirmed_userdata()
{
    //DIR = 1; PRM = 1; FCB = ?; FCV = 1; FC = 3;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB|userdata...

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = (byte)(5 + _send_userdata_len);
    _send_frame[3] = 0xd0 | (_NFCB << 5 & 0x20) | 0x02;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;
    push_userdata_to_frame();

    if (_debug) *_debug_stream << "[DataLink]Send confirmed userdata\n";

    send_frame(); //Send frame
}


void DataLinkMaster::unconfirmed_userdata()
{
    //DIR = 1; PRM = 1; FCB = ?; FCV = 0; FC = 4;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB|userdata...

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = (byte)(5 + _send_userdata_len);
    _send_frame[3] = 0xc4;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;
    push_userdata_to_frame();

    if (_debug) *_debug_stream << "[DataLink]Send unconfirmed userdata\n";

    send_frame(); //Send frame
}


void DataLinkMaster::ack()
{
    //DIR = 1; PRM = 0; FCB = 0; FCV = 0; FC = 0;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = 0x05;
    _send_frame[3] = 0x80;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short __crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(__crc);
    _send_frame[9] = get_high_byte_uint16(__crc);

    _send_frame_len = 10;

    if (_debug) *_debug_stream << "[DataLink]Ack\n";

    send_frame(); //Send frame
}


void DataLinkMaster::nack()
{
    //DIR = 1; PRM = 0; FCB = 0; FCV = 0; FC = 1;
    //Start_2|Length_1|Control_1|Dest_LSB|Dest_MSB|Source_LSB|Source_MSB|CRC__LSB|CRC_MSB

    _send_frame[0] = 0x05;
    _send_frame[1] = 0x64;
    _send_frame[2] = 0x05;
    _send_frame[3] = 0x81;
    _send_frame[4] = get_low_byte_uint16(_dest_address);
    _send_frame[5] = get_high_byte_uint16(_dest_address);
    _send_frame[6] = get_low_byte_uint16(_src_address);
    _send_frame[7] = get_high_byte_uint16(_src_address);

    unsigned short crc = create_crc(_send_frame, 0, 8);
    _send_frame[8] = get_low_byte_uint16(crc);
    _send_frame[9] = get_high_byte_uint16(crc);

    _send_frame_len = 10;

    if (_debug) *_debug_stream << "[DataLink]Nack\n";

    send_frame(); //Send frame
}


void DataLinkMaster::push_userdata_to_frame()
{
    //Frame = 10Bytes DataLink Header|Block1-Data (16Bytes)|CRC1|...| Blockn-Data (<16Bytes)|CRCn
    if (_send_userdata_len <= 0) return;
    unsigned short __crc;
    int __offset = 0;

    /* Caculate CRC and copy Block-16bytes */
    int __block16_total = _send_userdata_len / 16;
    for (int i = 0; i < __block16_total; i++)
    {
        __crc = create_crc(_send_userdata, __offset, 16);
        for (int j = 0; j < 16; j++)
        {
            _send_frame[_send_frame_len] = _send_userdata[__offset];
            _send_frame_len++;
            __offset++;
        }
        _send_frame[_send_frame_len++] = get_low_byte_uint16(__crc);
        _send_frame[_send_frame_len++] = get_high_byte_uint16(__crc);
    }

    /* Caculate CRC and copy remaining bytes (block < 16bits) */
    int __remain = _send_userdata_len % 16;
    if (__remain > 0)
    {
        __crc = create_crc(_send_userdata, __offset, __remain);
        for (int j = 0; j < __remain; j++)
        {
            _send_frame[_send_frame_len] = _send_userdata[__offset];
            _send_frame_len++;
            __offset++;
        }
        _send_frame[_send_frame_len++] = get_low_byte_uint16(__crc);
        _send_frame[_send_frame_len++] = get_high_byte_uint16(__crc);
    }
}


void DataLinkMaster::pop_userdata_from_frame()
{
    //Frame = 10Bytes DataLink Header|Block1-Data (16Bytes)|CRC1|...| Blockn-Data (<16Bytes)|CRCn

    _rcv_userdata_len = 0;
    int __offset = 10;

    /* Copy Block-16bytes */
    int __block16_total = (_rcv_frame_len - 10) / 18;
    for (int i = 0; i < __block16_total; i++)
    {
        for (int j = 0; j < 16; j++)
        {
            _rcv_userdata[_rcv_userdata_len] = _rcv_frame[__offset];
            _rcv_userdata_len++;
            __offset++;
        }
        __offset += 2; //ignore CRCi;
    }

    /* Copy remaining bytes (Block < 16 bytes) */
    int __remain = ((_rcv_frame_len - 10) % 18) - 2; // Khong tinh 2 byte CRC cuoi
    if (__remain > 0)
    {
        for (int j = 0; j < __remain; j++)
        {
            _rcv_userdata[_rcv_userdata_len] = _rcv_frame[__offset];
            _rcv_userdata_len++;
            __offset++;
        }
    }
}


void DataLinkMaster::send_frame()
{
    if (send_frame_event)
    {
        if (_debug)
        {
            *_debug_stream << "[DataLink]++++>";
            *_debug_stream << std::hex << std::setfill('0');
            for (int i = 0; i < _send_frame_len; i++)
                *_debug_stream << " " << std::setw(2) << (int)_send_frame[i];
            *_debug_stream << "\n";
        }
        FrameDataEventArg e;
        e.frame = _send_frame;
        e.length = _send_frame_len;

#ifdef LINUX_ONLY
        timeb __utc_now;
        ftime(&__utc_now);
        _last_send_time = (long long int)(__utc_now.time)*1000 + __utc_now.millitm;
#else
        milliseconds __utc_now = duration_cast<milliseconds>(system_clock::now().time_since_epoch());
        _last_send_time = __utc_now.count();
#endif // LINUX_ONLY

        send_frame_event->notify(this, &e);

    }

}

void DataLinkMaster::notify_userdata()
{
    if (segment_received_event)
    {
        SegmentDataEventArg e;
        e.segment = _rcv_userdata;
        e.length = _rcv_userdata_len;
        segment_received_event->notify(this, &e);
    }
}

}
