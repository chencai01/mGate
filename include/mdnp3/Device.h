#ifndef DEVICE_H_INCLUDED
#define DEVICE_H_INCLUDED

#ifdef LINUX_ONLY
#include <sys/timeb.h>
#else
#include <chrono>
#endif // LINUX_ONLY

#include "mdnp3/ApplicationMaster.h"

namespace Dnp3Master
{

namespace DevRqsCode
{
enum DevRqsCode
{
    undefined, class0_all, class1_all, class2_all, class3_all, integrity_poll, timesync,
    disable_uns_class1, disable_uns_class2, disable_uns_class3,
    enable_uns_class1, enable_uns_class2, enable_uns_class3,
    cmd_crob, cmd_ao16, cmd_ao32
};
}

namespace DevStatusCode
{
enum DevStatusCode {Bad=0, Good=1};
}

namespace DevOpStatusCode
{
enum DevOpStatusCode
{
    idle, startup, poll_ready, time_sync, startup_waiting, poll_waiting,
    cmd_s1_waiting, cmd_s2_waiting, time_s1_waiting, time_s2_waiting
};
}

class Device
{
public:
    Device();
    virtual ~Device();

    void start();
    void stop();
    void deep_stop();
    void write_cmd_point(Dnp3Point& point);

    Dnp3PointDataEvent* dnp3_point_rcv_event;


    void set_name(std::string& name)
    {
        _name = name;
    }

    std::string get_name()
    {
        return _name;
    }

    void set_description(std::string& desc)
    {
        _description = desc;
    }

    std::string get_description()
    {
        return _description;
    }

    DevStatusCode::DevStatusCode get_status()
    {
        return _dev_status;
    }

    DevOpStatusCode::DevOpStatusCode get_op_status()
    {
        return _dev_oper_status;
    }

    void set_enable(bool enabled)
    {
        _enabled = enabled;
    }

    void set_poll(bool class0_en, bool class1_en, bool class2_en, bool class3_en)
    {
        _poll_class0_enabled = class0_en;
        _poll_class1_enabled = class1_en;
        _poll_class2_enabled = class2_en;
        _poll_class3_enabled = class3_en;
    }

    void set_uns(bool class1_en, bool class2_en, bool class3_en)
    {
        _uns_class1_enabled = class1_en;
        _uns_class2_enabled = class2_en;
        _uns_class3_enabled = class3_en;
    }

    void set_timesync_enable(bool enabled)
    {
        _timesync_enabled = enabled;
    }

    void set_poll_count_max(int max_val)
    {
        _poll_count_max = max_val;
    }

    ApplicationMaster* get_app_loader()
    {
        return _application_loader;
    }

    TransportMaster* get_trans_loader()
    {
        return _transport_loader;
    }

    DataLinkMaster* get_datalink_loader()
    {
        return _datalink_loader;
    }

    void set_time_ready(long time_value)
    {
        _ready_timer.set_time(time_value);
    }

    long get_time_ready()
    {
        return _ready_timer.get_time();
    }

    void set_class0_time(long time_value)
    {
        _class0_timer.set_time(time_value);
    }

    long get_class0_time()
    {
        return _class0_timer.get_time();
    }

    void set_debug(bool dev=false, bool ap=false, bool trans=false, bool dl=false)
    {
        _debug = dev;
        _application_loader->set_debug(ap);
        _transport_loader->set_debug(trans);
        _datalink_loader->set_debug(dl);
    }

    void set_ddev(bool val=false)
    {
        _debug = val;
    }

    void set_dap(bool val=false)
    {
        _application_loader->set_debug(val);
    }

    void set_dtr(bool val=false)
    {
        _transport_loader->set_debug(val);
    }

    void set_ddl(bool val=false)
    {
        _datalink_loader->set_debug(val);
    }

    void set_debug_stream(std::stringstream& debug_stream)
    {
        _debug_stream = &debug_stream;
        _application_loader->set_debug_stream(debug_stream);
        _transport_loader->set_debug_stream(debug_stream);
        _datalink_loader->set_debug_stream(debug_stream);
    }

private:

    ApplicationMaster* _application_loader;
    TransportMaster* _transport_loader;
    DataLinkMaster* _datalink_loader;

    std::string _name;
    std::string _description;
    DevStatusCode::DevStatusCode _dev_status;
    DevOpStatusCode::DevOpStatusCode _dev_oper_status;

    bool _enabled;
    bool _timesync_enabled;
    bool _poll_class0_enabled;
    bool _poll_class1_enabled;
    bool _poll_class2_enabled;
    bool _poll_class3_enabled;
    bool _uns_class1_enabled;
    bool _uns_class2_enabled;
    bool _uns_class3_enabled;
    bool _startup_required;

    int _poll_count_max;
    int _poll_count;
    Timer _ready_timer; // Thoi gian giua 2 lan gui request
    Timer _class0_timer; // Thoi gian giua 2 lan gui poll class 0

    DevRqsCode::DevRqsCode _startup_curr_rqs;
    DevRqsCode::DevRqsCode _poll_curr_rqs;
    std::queue<Dnp3Point> _cmd_point_list;

    bool _debug;
    std::stringstream* _debug_stream;

    void proc_startup();
    void proc_poll();
    void proc_cmd();
    void proc_timesync();
    void exec_cmd_crob();
    void exec_cmd_ao();

    /*
    Gia tri co the tra ve tu check_timer
    1: timer is not timeout
    0: timer is timeout
    -1: timer is not running
    */
    int check_timer();

    void check_IIN();

    typedef EventHandler<Device, void, Dnp3PointEventArg> Dnp3PointEventHandler;
    Dnp3PointEventHandler* _dnp3_point_rcv_handler;
    Dnp3PointEventHandler* _dnp3_point_uns_rcv_handler;
    Dnp3PointEventHandler* _dnp3_cmd_echo_handler;
    Dnp3PointEventHandler* _dnp3_time_delay_handler;

    void on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e);
    void on_dnp3_point_uns_rcv_event(void* sender, Dnp3PointEventArg* e);
    void on_dnp3_cmd_echo_event(void* sender, Dnp3PointEventArg* e);
    void on_dnp3_time_delay_event(void* sender, Dnp3PointEventArg* e);
};


} // Dnp3Master namespace


#endif // DEVICE_H_INCLUDED


