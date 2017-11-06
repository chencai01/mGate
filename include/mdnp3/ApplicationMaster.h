#ifndef APPLICATIONMASTER_H_INCLUDED
#define APPLICATIONMASTER_H_INCLUDED

#include "mdnp3/TransportMaster.h"
#include "mdnp3/ApplicationResponseHeader.h"
#include "mdnp3/Dnp3PointEventArg.h"
#include "mdnp3/Dnp3Points.h"


namespace Dnp3Master
{
typedef std::vector<Dnp3Master::Dnp3Point> Dnp3PointVector;

namespace APOpStatusCode
{
enum APOpStatusCode
{
    Idle, AwaitFirst, Assembly
};
}

namespace APUNSOpStatusCode
{
enum APUNSOpStatusCode
{
    Startup, FirstUR, Idle
};
}


class ApplicationMaster
{
public:
    ApplicationMaster();
    virtual ~ApplicationMaster();

    /*
    Event nay duoc su dung voi nhung diem du lieu dnp3 thong thuong
    */
    Dnp3PointDataEvent* dnp3_point_rcv_event;

    /*
    unsolicited data se duoc tach rieng ra event khac
    */
    Dnp3PointDataEvent* dnp3_point_uns_rcv_event;

    /*
    Event nay phat sinh khi slave phan hoi trang thai lenh dieu khien
    Master su dung event nay de kiem tra lenh dieu khien co thanh cong hay khong
    Master su dung event nay de thuc hien buoc lenh thu hai khi lenh la select/operate
    */
    Dnp3PointDataEvent* dnp3_cmd_echo_event;

    /*
    Master su dung event nay khi dong bo thoi gian voi slave (xem them tai lieu DNP3)
    Khong co du lieu nao gan kem voi event nay, neu master nhan duoc event nay
    thi can su dung function get_time_delay de lay gia tri time_delay
    */
    Dnp3PointDataEvent* dnp3_time_delay_event;

    void connect_to_transport(TransportMaster* tp);

    // Khi application layer timeout thi can thuc hien mot so cong viec thong qua function nay
    void act_on_timeout();

    void reset_layer_state();

    long get_timeout()
    {
        return _rsp_timeout.get_time();
    }

    void set_timeout(long value)
    {
        _rsp_timeout.set_time(value);
    }

    bool is_timeout()
    {
        return _rsp_timeout.is_timeout();
    }

    bool timer_is_running()
    {
        return _rsp_timeout.is_running();
    }

    ApplicationResponseHeader get_rsp_header()
    {
        return _rcv_header;
    }

    unsigned long long get_time_delay()
    {
        return _time_delay;
    }

    APOpStatusCode::APOpStatusCode get_operation_status()
    {
        return _ap_oper_status;
    }

    void set_operation_status(APOpStatusCode::APOpStatusCode status_code)
    {
        _ap_oper_status = status_code;
    }

    APUNSOpStatusCode::APUNSOpStatusCode get_uns_operation_status()
    {
        return _ap_uns_oper_status;
    }

    void set_uns_operation_status(APUNSOpStatusCode::APUNSOpStatusCode status_code)
    {
        _ap_uns_oper_status = status_code;
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
    TransportMaster* _transport_loader;
    Timer _rsp_timeout;
    int _lastfrag_index;                            // The first byte index of the last fragment in vector, used when compare Octet-by-octet;
    unsigned long long _time_cto;                   // Save time value received with obj grp 51 and represent as a header time for event with relative time
    unsigned long long _time_delay;                 // Setted when receive object group 51 and used when need to sync time

    byte _send_fragment[MAXFRAGMENT];
    int _send_fragment_len;

    ByteVector _asdu;
    ByteVector _asdu_uns;
    Dnp3PointVector _point_list;

    ApplicationRequestHeader _send_header;          // Header value of the last sent request fragment
    ApplicationResponseHeader _rcv_header;          // Header value of the last accepted received fragment (not uns)
    ApplicationResponseHeader _rcv_header_uns;      // Header value of the last accepted received uns fragment
    ApplicationResponseHeader _rcv_new_header;      // Header value of the last received fragment (may be uns or not)

    APOpStatusCode::APOpStatusCode _ap_oper_status;
    APUNSOpStatusCode::APUNSOpStatusCode _ap_uns_oper_status;

    bool _debug;
    std::stringstream* _debug_stream;

    /*
    Use this event handler to receive fragment data from transport layer
    */
    typedef EventHandler<ApplicationMaster, void, FragmentDataEventArg> FragmentDataEventHandler;
    FragmentDataEventHandler* _fragment_rcv_event_handler;
    void on_fragment_received(void* sender, FragmentDataEventArg* e);

    inline void notify_points(bool is_uns);

    void send_debug();

    /*********PROCESS FRAGMENT BASED ON OPERATION STATE OF APPLICATION LAYER*********/

    void proc_frag_awaitfirst_state(ByteVector* buff, ByteVector::size_type length);
    void proc_frag_assembly_state(ByteVector* buff, ByteVector::size_type length);

    void proc_frag_uns_startup_state(ByteVector* buff, ByteVector::size_type length);
    void proc_frag_uns_first_ur_state(ByteVector* buff, ByteVector::size_type length);
    void proc_frag_uns_idle_state(ByteVector* buff, ByteVector::size_type length);

    /*********************************************************************************/



    /*********ANALYZE ASDU AND QUALIFIER TO DETACH DNP3 POINTS*********/

    void proc_asdu();
    void proc_asdu_uns();

    void proc_qualifier_0x00(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff);
    void proc_qualifier_0x01(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff);
    void proc_qualifier_0x07(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff);
    void proc_qualifier_0x17(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff);
    void proc_qualifier_0x28(ByteVector::size_type& idx, DataObjectCode::DataObjectCode& obj, ByteVector& buff);

    /*******************************************************************/


    void detach_bi_packed(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_bi_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_bo_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_counter32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_counter16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_counter32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_counter16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_fcounter32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_fcounter16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_fcounter32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_fcounter16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_ai32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_ai16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_ai32(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_ai16(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_ai32float_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_ai64float_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_ao16_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);
    void detach_ao32_flags(ByteVector::size_type& idx, unsigned int start, unsigned int stop, ByteVector& buff);

    void detach_bi_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_bi_event_abstime(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_bi_event_rltvtime(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_counter32_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_counter16_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_fcounter32_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_fcounter16_event_flags(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_ai32_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ai16_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ai32_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ai16_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_ai32float_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ai64float_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_ai32float_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ai64float_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_bocmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_bocmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_bocmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_ao16cmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ao16cmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ao16cmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_ao32cmd_echo(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ao32cmd_event(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);
    void detach_ao32cmd_event_time(ByteVector::size_type& idx, unsigned int prefixSize, unsigned int count, ByteVector& buff);

    void detach_timedatecto_abstime_sync(ByteVector::size_type& idx, unsigned int count, ByteVector& buff);
    void detach_timedatecto_abstime_unsync(ByteVector::size_type& idx, unsigned int count, ByteVector& buff);
    void detach_timedelay_coarse(ByteVector::size_type& idx, unsigned int count, ByteVector& buff);
    void detach_timedelay_fine(ByteVector::size_type& idx, unsigned int count, ByteVector& buff);

public:
    /***********LIST OF SERVICES PROVIDED BY APPLICATION LAYER***********/


    /*
    Read class data:
    isClass = 1: read class 1 only
    isClass = 2: read class 2 only
    isClass = 3: read class 3 only
    isClass = 123: read class 1,2,3 in one request
    */
    bool read_class_data(int isClass, bool isConfirm);

    /*
    Read class data:
    isClass = 1: read class 1 only
    isClass = 2: read class 2 only
    isClass = 3: read class 3 only
    */
    bool read_class_data(int isClass, unsigned int count, bool isConfirm);

    bool send_confirmed(int seq, bool isUNS);
    bool request_integrity_poll(bool isConfirm);
    bool clear_restart_bit(bool isConfirm);
    bool request_cold_restart(bool isConfirm);
    bool delay_measurement(bool isWait, bool isConfirm);
    bool record_current_time(bool isWait, bool isConfirm);
    bool write_abstime(unsigned long long value, bool isWait, bool isConfirm);
    bool write_last_recorded_time(unsigned long long value, bool isWait, bool isConfirm);
    bool read_time_and_date(bool isWait, bool isConfirm);

    // isClass: 1, 2, 3, 123
    bool set_enable_uns(int isClass, bool isWait, bool isConfirm);

    // isClass: 1, 2, 3, 123
    bool set_disable_uns(int isClass, bool isWait, bool isConfirm);

    // cmdType: 1(Select), 2(Operate), 3(Direct), 4(DirectNoAck)
    bool send_cmd_crob(Dnp3Point* point, int cmdType, bool isWait, bool isConfirm);

    // cmdType: 1(Select), 2(Operate), 3(Direct), 4(DirectNoAck)
    bool send_cmd_ao(Dnp3Point* point, int cmdType, bool isWait, bool isConfirm);

    /************************************************************************/
};

} // Dnp3Master namespace

#endif // APPLICATIONMASTER_H_INCLUDED

