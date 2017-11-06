#ifndef DATALINKMASTER_H_INCLUDED
#define DATALINKMASTER_H_INCLUDED

#ifdef LINUX_ONLY
#include <sys/timeb.h>
#else
#include <chrono>
#endif // LINUX_ONLY

#include "mdnp3/DataLinkHeader.h"
#include "mdnp3/Utility.h"
#include "mdnp3/Timer.h"
#include "mdnp3/FrameDataEventArg.h"
#include "mdnp3/SegmentDataEventArg.h"

namespace Dnp3Master
{

namespace LinkOpStatusCode
{
enum LinkOpStatusCode
{
    SecUnResetIdle, SecResetIdle, ResetLinkWait_1, ResetLinkWait_2,
    UR_LinkStatusWait, TestWait, CfmDataWait, R_LinkStatusWait
};
}

class DataLinkMaster
{
public:
    DataLinkMaster();
    virtual ~DataLinkMaster();

    Dnp3SegmentDataEvent* segment_received_event; // Forward segment data to transport layer
    Dnp3FrameDataEvent* send_frame_event; // Forward request frame data to physical layer

    bool send_unconfirmed_userdata(byte usdata[], int length);
    bool send_confirmed_userdata(byte usdata[], int length);
    bool send_test_link_state();

    // Receive frame data from physical layer
    void on_frame_received(void* sender, FrameDataEventArg* e);

    // Khi datalink layer timeout thi can thuc hien mot so cong viec thong qua function nay
    void act_on_timeout();

    void reset_layer_state();

    unsigned short get_src_address()
    {
        return _src_address;
    }
    void set_src_address(unsigned short source)
    {
        _src_address = source;
    }

    unsigned short get_dest_address()
    {
        return _dest_address;
    }
    void set_dest_address(unsigned short dest)
    {
        _dest_address = dest;
    }

    long get_timeout()
    {
        return _rsp_timer.get_time();
    }

    void set_timeout(long value)
    {
        _rsp_timer.set_time(value);
    }

    bool is_timeout()
    {
        return _rsp_timer.is_timeout();
    }

    bool timer_is_running()
    {
        return _rsp_timer.is_running();
    }

    bool is_link_reset()
    {
        return _sec_station_is_reset;
    }

    bool is_link_overflow()
    {
        return _sec_station_is_overflow;
    }

    int get_retry_max()
    {
        return _retry_max;
    }

    void set_retry_max(int value)
    {
        _retry_max = value;
    }

    LinkOpStatusCode::LinkOpStatusCode get_operation_status()
    {
        return _oper_status;
    }

    void set_operation_status(LinkOpStatusCode::LinkOpStatusCode status_code)
    {
        _oper_status = status_code;
    }

    long long int get_last_send_time()
    {
        return _last_send_time;
    }

    long long int get_last_rcv_time()
    {
        return _last_rcv_time;
    }

    void set_debug(bool debug_value)
    {
        _debug = debug_value;
    }

    void set_debug_stream(std::stringstream& debug_stream)
    {
        _debug_stream = &debug_stream;
    }

private:

    bool _sec_station_is_reset;
    bool _sec_station_is_overflow;
    LinkOpStatusCode::LinkOpStatusCode _oper_status;
    int _retry_max;
    int _retry_count;
    int _NFCB; //Next frame count bit
    DataLinkHeader _rcv_header;
    Timer _rsp_timer;
    long long int _last_send_time;
    long long int _last_rcv_time;

    unsigned short _src_address;
    unsigned short _dest_address;

    byte _send_frame[MAXFRAME]; //Buffer for sending frame to physical layer
    int _send_frame_len;

    byte _rcv_frame[MAXFRAME]; //Buffer for receiving frame from physical layer
    int _rcv_frame_len;

    byte _send_userdata[MAXFRAME]; //Buffer for saving userdata request
    int _send_userdata_len;

    byte _rcv_userdata[MAXFRAME]; //Buffer for saving userdata from Outstation
    int _rcv_userdata_len;

    bool _debug;
    std::stringstream* _debug_stream;

    void proc_frame_userdata();
    void proc_frame_sec_unreset_idle();
    void proc_frame_sec_reset_idle();
    void proc_frame_resetlink_wait1();
    void proc_frame_resetlink_wait2();
    void proc_frame_UR_linkstatus_wait();
    void proc_frame_test_wait();
    void proc_frame_cfmdata_wait();
    void proc_frame_R_linkstatus_wait();

    void reset_link_state();
    void request_link_status();
    void test_link_state();
    void confirmed_userdata();
    void unconfirmed_userdata();
    void ack();
    void nack();

    void push_userdata_to_frame();
    void pop_userdata_from_frame();

    inline void send_frame();
    inline void notify_userdata();

};

} // Dnp3Master namespace


#endif // DATALINKMASTER_H_INCLUDED

