#include "mdnp3/pch.h"
#include "mdnp3/TransportMaster.h"

namespace Dnp3Master
{

TransportMaster::TransportMaster()
{
    _segment_data_event_handler = new Dnp3SegmentDataEventHandler();
    _segment_data_event_handler->connect(this, &TransportMaster::on_segment_received);
    _datalink_loader = NULL;
    fragment_received_event = NULL;
    _oper_status = TPOpStatusCode::Idle;
    _last_seg_index = 0;
    _rcv_expected_seq = 0;
    _rcv_last_seq = 0;
    _send_segment_len = 0;
    _debug = false;
    _debug_stream = NULL;
}

TransportMaster::~TransportMaster()
{
    _datalink_loader->segment_received_event = NULL;
    _segment_data_event_handler->disconnect();
    delete _segment_data_event_handler;
    _segment_data_event_handler = NULL;
    _datalink_loader = NULL;
}


void TransportMaster::connect_to_datalink(DataLinkMaster* dlink)
{
    _datalink_loader = dlink;
    _datalink_loader->segment_received_event = _segment_data_event_handler;
}


bool TransportMaster::send_userdata_request(byte us[], int length, bool confirmed)
{
    /*==== Break TSDU into multi TPDU to send ====*/

    int __max_len = MAXSEGMENT - 1;
    int __lastseg_len = length % __max_len;
    int __seg_total = length / __max_len;
    if (__lastseg_len != 0)
    {
        __seg_total += 1;
    }
    else __lastseg_len = __max_len;

    /*=============================================*/

    bool __ret = false;
    int __idx = 0; // Chi so byte dau tien cua segment trong userdata
    int __len = 0; // So byte cua segment trong 1 lan gui
    bool __fir;
    bool __fin;
    _send_segment_len = 0;
    for (int i = 0; i < __seg_total; i++)
    {
        /*
        Segment dau tien va cuoi cung la truong hop dac biet khi gui nhieu segment, mot so dieu can quan tam la:
        FIR=1 neu la segment dau tien
        FIN=1 neu la segment cuoi cung
        Khi FIR=1 va chi co 1 segment duy nhat thi FIN=1 va do dai segment se la __lastseg_len
        Khi FIR=1 va co nhieu hon 1 segment thi do dai segment se la __max_len
        Truong hop FIN=1 thi mot la chi co duy nhat 1 segment, hai la segment cuoi cung, do dai segment laf __lastseg_len
        */
        __fir = (i == 0);
        __fin = (__seg_total == 1) | (i == (__seg_total - 1));

        if (__fin) __len = __lastseg_len;
        else __len = __max_len;

        _send_header.set_fir(__fir);
        _send_header.set_fin(__fin);
        _send_header.set_seq(_send_header.get_next_seq());
        _send_segment[0] = _send_header.get_header();

        __idx = __len * i;

        for (int j = 0; j < __len; j++)
        {
            _send_segment[j + 1] = us[__idx+j];
        }
        _send_segment_len = __len + 1;

        if (_debug)
        {
            *_debug_stream << "[Transport]~~~~>";
            *_debug_stream << std::hex << std::setfill('0');
            for (int i = 0; i < _send_segment_len; i++)
                *_debug_stream << " " << std::setw(2) << (unsigned short)_send_segment[i];
            *_debug_stream
                    << std::dec     << "\n[Transport header request]:\n"
                    << "FIR: "      << _send_header.get_fir()
                    << ", FIN: "    << _send_header.get_fin()
                    << ", SEQ: "    << (int)_send_header.get_seq()
                    << "\n";
        }

        if (confirmed)
            __ret = _datalink_loader->send_confirmed_userdata(_send_segment, _send_segment_len);
        else
            __ret = _datalink_loader->send_unconfirmed_userdata(_send_segment, _send_segment_len);
    }

    return __ret;
}


void TransportMaster::on_segment_received(void* sender, SegmentDataEventArg* e)
{
    _rcv_header = e->segment[0];

    if (_debug)
    {
        *_debug_stream << "[Transport]<~~~~";
        *_debug_stream << std::hex << std::setfill('0');
        for (int i = 0; i < e->length; i++)
            * _debug_stream << " " << std::setw(2) << (int)(e->segment[i]);

        *_debug_stream
                << std::dec     << "\n[Transport header response]:\n"
                << "FIR: "      << _rcv_header.get_fir()
                << ", FIN: "    << _rcv_header.get_fin()
                << ", SEQ: "    << (int)_rcv_header.get_seq()
                << "\n";
    }
    switch (_oper_status)
    {
    case TPOpStatusCode::Idle:
        proc_segment_idlestate(e->segment, e->length);
        break;
    case TPOpStatusCode::Assembly:
        proc_segment_assemblystate(e->segment, e->length);
        break;
    default:
        break;
    }
}


void TransportMaster::proc_segment_idlestate(byte buff[], int length)
{
    if (_rcv_header.get_fir())
    {
        if (_rcv_header.get_fin()) //Entire application data fits winthin the segment (FIR=1, FIN=1, SEQ=X)
        {
            _rcv_fragment.clear();
            _last_seg_index = 0;
            for (int i = 1; i < length; i++)
            {
                _rcv_fragment.push_back(buff[i]);
            }
            notify_frame();
        }
        else //First of multiple segments (FIR=1, FIN=0, SEQ=X
        {
            _oper_status = TPOpStatusCode::Assembly;
            _rcv_fragment.clear();
            _last_seg_index = 0;
            _rcv_last_seq = (int)_rcv_header.get_seq();
            _rcv_expected_seq = _rcv_header.get_next_seq();
            for (int i = 1; i < length; i++)
            {
                _rcv_fragment.push_back(buff[i]);
            }
        }
    }
}


void TransportMaster::proc_segment_assemblystate(byte buff[], int length)
{
    if (_rcv_header.get_fir())
    {
        if (_rcv_header.get_fin()) //Entire application data fits within the segment
        {
            _oper_status = TPOpStatusCode::Idle;
            _rcv_fragment.clear();
            _last_seg_index = 0;
            for (int i = 1; i < length; i++)
            {
                _rcv_fragment.push_back(buff[i]);
            }
            notify_frame();
        }
        else //First of multiple segments (FIR=1, FIN=0, SEQ=X
        {
            _rcv_fragment.clear();
            _last_seg_index = 0;
            _rcv_last_seq = (int)_rcv_header.get_seq();
            _rcv_expected_seq = _rcv_header.get_next_seq();
            for (int i = 1; i < length; i++)
            {
                _rcv_fragment.push_back(buff[i]);
            }
        }
    }
    else //FIR=0
    {
        int __seq = (int)_rcv_header.get_seq();
        if (__seq == _rcv_last_seq) // received seq == previous received seq
        {
            int __lastseg_len = _rcv_fragment.size() - _last_seg_index;
            if (length != __lastseg_len)
            {
                _oper_status = TPOpStatusCode::Idle;
                return;
            }

            int __idx = _last_seg_index - 1;
            for (int i = 1; i < length; i++)
            {
                if (_rcv_fragment.at(__idx + i) != buff[i])
                {
                    _oper_status = TPOpStatusCode::Idle;
                    return;
                }
            }
        }
        else if (__seq == _rcv_expected_seq)
        {
            if (_rcv_header.get_fin()) //final segment
            {
                _oper_status = TPOpStatusCode::Idle;
                for (int i = 1; i < length; i++)
                {
                    _rcv_fragment.push_back(buff[i]);
                }
                notify_frame();
            }
            else //Not final segment
            {
                _rcv_last_seq = (int)_rcv_header.get_seq();
                _rcv_expected_seq = _rcv_header.get_next_seq();
                for (int i = 1; i < length; i++)
                {
                    _rcv_fragment.push_back(buff[i]);
                }
                _last_seg_index = _rcv_fragment.size() - (length - 1);
            }
        }
        else _oper_status = TPOpStatusCode::Idle;
    }
}


void TransportMaster::notify_frame()
{
    if (fragment_received_event)
    {
        FragmentDataEventArg __e;
        __e.fragment = &_rcv_fragment;
        __e.length = _rcv_fragment.size();
        fragment_received_event->notify(this, &__e);
    }
}

} // Dnp3Master namespace

