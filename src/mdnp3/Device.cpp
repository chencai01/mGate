#include "mdnp3/pch.h"
#include "mdnp3/Device.h"

namespace Dnp3Master
{

Device::Device()
{
    _dnp3_point_rcv_handler = new Dnp3PointEventHandler();
    _dnp3_point_uns_rcv_handler = new Dnp3PointEventHandler();
    _dnp3_cmd_echo_handler = new Dnp3PointEventHandler();
    _dnp3_time_delay_handler = new Dnp3PointEventHandler();

    _dnp3_point_rcv_handler->connect(this, &Device::on_dnp3_point_rcv_event);
    _dnp3_point_uns_rcv_handler->connect(this, &Device::on_dnp3_point_uns_rcv_event);
    _dnp3_cmd_echo_handler->connect(this, &Device::on_dnp3_cmd_echo_event);
    _dnp3_time_delay_handler->connect(this, &Device::on_dnp3_time_delay_event);

    _datalink_loader = new DataLinkMaster();
    _transport_loader = new TransportMaster();
    _application_loader = new ApplicationMaster();

    _transport_loader->connect_to_datalink(_datalink_loader);
    _application_loader->connect_to_transport(_transport_loader);

    _application_loader->dnp3_point_rcv_event = _dnp3_point_rcv_handler;
    _application_loader->dnp3_point_uns_rcv_event = _dnp3_point_uns_rcv_handler;
    _application_loader->dnp3_cmd_echo_event = _dnp3_cmd_echo_handler;
    _application_loader->dnp3_time_delay_event = _dnp3_time_delay_handler;

    _name = "dev";
    _description = "desc";
    dnp3_point_rcv_event = NULL;
    _startup_required = true;
    _poll_count = 0;
    _poll_count_max = 10;
    _ready_timer.set_time(100);
    _class0_timer.set_time(1);

    _dev_oper_status = DevOpStatusCode::idle;
    _startup_curr_rqs = DevRqsCode::undefined;
    _poll_curr_rqs = DevRqsCode::undefined;
    _dev_status = DevStatusCode::Bad;
    _enabled = false;
    _timesync_enabled = false;
    _debug = false;
    _debug_stream = NULL;
    _poll_class0_enabled = false;
    _poll_class1_enabled = false;
    _poll_class2_enabled = false;
    _poll_class3_enabled = false;
    _uns_class1_enabled = false;
    _uns_class2_enabled = false;
    _uns_class3_enabled = false;

}

Device::~Device()
{
    _application_loader->dnp3_point_rcv_event = NULL;
    _application_loader->dnp3_point_uns_rcv_event = NULL;
    _application_loader->dnp3_cmd_echo_event = NULL;
    _application_loader->dnp3_time_delay_event = NULL;

    _dnp3_point_rcv_handler->disconnect();
    _dnp3_point_uns_rcv_handler->disconnect();
    _dnp3_cmd_echo_handler->disconnect();
    _dnp3_time_delay_handler->disconnect();

    delete _application_loader;
    delete _transport_loader;
    delete _datalink_loader;

    delete _dnp3_time_delay_handler;
    delete _dnp3_cmd_echo_handler;
    delete _dnp3_point_rcv_handler;

}

void Device::start()
{
    if (!_enabled)
    {
        stop();
        return;
    }

    if (_ready_timer.is_running())
    {
        if (_ready_timer.is_timeout())
            _ready_timer.start();
        else
            return;
    }
    else
        _ready_timer.start();

    if (_debug)
        *_debug_stream << "DevOpCode:" << _dev_oper_status << "\n";

    /** LUA CHON CHE DO THUC HIEN **/

    switch (_dev_oper_status)
    {
    case DevOpStatusCode::idle:
        if (_startup_required)
        {
            _dev_oper_status = DevOpStatusCode::startup;
            proc_startup();
            break;
        }
        _dev_oper_status = DevOpStatusCode::poll_ready;
        proc_poll();
        break;
    case DevOpStatusCode::startup:
        proc_startup();
        break;
    case DevOpStatusCode::poll_ready:
        proc_poll();
        break;
    case DevOpStatusCode::time_sync:
        proc_timesync();
        break;
    default:
        check_timer();
        break;
    }
}

void Device::stop()
{
    _dev_oper_status = DevOpStatusCode::idle;
    _startup_curr_rqs = DevRqsCode::undefined;
    _poll_curr_rqs = DevRqsCode::undefined;
    _poll_count = 0;
}

void Device::deep_stop()
{
    stop();
    _class0_timer.stop();
    _ready_timer.stop();
    _application_loader->reset_layer_state();
    _datalink_loader->reset_layer_state();
}

void Device::write_cmd_point(Dnp3Point& point)
{
    switch (point.type_id)
    {
    case PointTypeCode::bocmd:
        _cmd_point_list.push(point);
        break;

    case PointTypeCode::ao16cmd:
        _cmd_point_list.push(point);
        break;

    case PointTypeCode::ao32cmd:
        _cmd_point_list.push(point);
        break;

    default:
        break;
    }

    /*
    Neu dev dang khong lam gi (idle) thi co the thuc hien lenh ngay lap tuc
    */
    if (_dev_oper_status == DevOpStatusCode::idle)
        proc_cmd();

}

void Device::proc_startup()
{
    // Khong cho phep dieu khien khi startup
    while (!_cmd_point_list.empty())
    {
        _cmd_point_list.pop();
    }

    /** CAC BUOC THUC HIEN QUA TRINH STARTUP **/

    if (_debug)
        *_debug_stream << "StartUpRqs:" << _startup_curr_rqs << "\n";

    /* buoc 1: disable unsolicited message class 1 --------- */
    if (_startup_curr_rqs == DevRqsCode::undefined)
    {
        if (_application_loader->set_disable_uns(1, true, false))
        {
            _startup_curr_rqs = DevRqsCode::disable_uns_class1;
            _dev_oper_status = DevOpStatusCode::startup_waiting;
        }
        return;
    }

    /* buoc 2: disable unsolicited message class 2 --------- */
    if (_startup_curr_rqs == DevRqsCode::disable_uns_class1)
    {
        if (_application_loader->set_disable_uns(2, true, false))
        {
            _startup_curr_rqs = DevRqsCode::disable_uns_class2;
            _dev_oper_status = DevOpStatusCode::startup_waiting;
        }
        return;
    }

    /* buoc 3: disable unsolicited message class 3 --------- */
    if (_startup_curr_rqs == DevRqsCode::disable_uns_class2)
    {
        if (_application_loader->set_disable_uns(3, true, false))
        {
            _startup_curr_rqs = DevRqsCode::disable_uns_class3;
            _dev_oper_status = DevOpStatusCode::startup_waiting;
        }
        return;
    }

    /* buoc 4: enable uns message class 1 (neu duoc) ------- */
    if (_startup_curr_rqs == DevRqsCode::disable_uns_class3)
    {
        if (_uns_class1_enabled)
        {
            if (_application_loader->set_enable_uns(1, true, false))
            {
                _startup_curr_rqs = DevRqsCode::enable_uns_class1;
                _dev_oper_status = DevOpStatusCode::startup_waiting;
            }
            return;
        }
        else
        {
            _startup_curr_rqs = DevRqsCode::enable_uns_class1;   // Chuyen sang buoc 5 luon
        }
    }

    /* buoc 5: enable uns message class 2 (neu duoc) ------- */
    if (_startup_curr_rqs == DevRqsCode::enable_uns_class1)
    {
        if (_uns_class2_enabled)
        {
            if (_application_loader->set_enable_uns(2, true, false))
            {
                _startup_curr_rqs = DevRqsCode::enable_uns_class2;
                _dev_oper_status = DevOpStatusCode::startup_waiting;
            }
            return;
        }
        else
        {
            _startup_curr_rqs = DevRqsCode::enable_uns_class2;
        }
    }

    /* buoc 6: enable uns message class 3 (neu duoc) ------- */
    if (_startup_curr_rqs == DevRqsCode::enable_uns_class2)
    {
        if (_uns_class3_enabled)
        {
            if (_application_loader->set_enable_uns(3, true, false))
            {
                _startup_curr_rqs = DevRqsCode::enable_uns_class3;
                _dev_oper_status = DevOpStatusCode::startup_waiting;
            }
            return;
        }
        else
        {
            _startup_curr_rqs = DevRqsCode::enable_uns_class3;
        }
    }

    /* buoc 7: Request integrity poll ---------------------- */
    if (_startup_curr_rqs == DevRqsCode::enable_uns_class3)
    {
        if (_application_loader->request_integrity_poll(false))
        {
            _startup_curr_rqs = DevRqsCode::integrity_poll;
            _dev_oper_status = DevOpStatusCode::startup_waiting;
        }
        return;
    }

    /* buoc 8: San sang chuyen sang giai doan poll --------- */
    if (_startup_curr_rqs == DevRqsCode::integrity_poll)
    {
        _startup_curr_rqs = DevRqsCode::undefined;
        _poll_curr_rqs = DevRqsCode::undefined;
        _dev_oper_status = DevOpStatusCode::poll_ready;
        _startup_required = false;

        /*
            Force application layer chuyen trang thai xu ly uns
            (vi da thuc hien integrity poll roi)
        */
        if (_application_loader->get_uns_operation_status() == APUNSOpStatusCode::Idle)
            _application_loader->set_uns_operation_status(APUNSOpStatusCode::FirstUR);

        return;
    }
}

void Device::proc_poll()
{
    if (!_cmd_point_list.empty())
    {
        proc_cmd(); // process command
        return;
    }

    if (_poll_count > _poll_count_max) // count excessed
    {
        stop();
        return;
    }

    /** CAC BUOC THUC HIEN QUA TRINH POLL **/

    if (_debug)
        *_debug_stream << "PollRqs:" << _poll_curr_rqs << "\n";

    /* Poll class 0 */
    if (_poll_curr_rqs == DevRqsCode::undefined)
    {
        if (_poll_class0_enabled)
        {
            if (!_class0_timer.is_running())
            {
                /*
                    Lan dau tien poll sau khi startup, bo qua class 0
                */
                _class0_timer.start();
                _poll_curr_rqs = DevRqsCode::class0_all; // break down
            }
            else if (_class0_timer.is_timeout())
            {
                if (_application_loader->read_class_data(0, false))
                {
                    _poll_curr_rqs = DevRqsCode::class0_all;
                    _dev_oper_status = DevOpStatusCode::poll_waiting;
                    _poll_count++;
                    _class0_timer.start();
                }
                return;
            }
            else
            {
                _poll_curr_rqs = DevRqsCode::class0_all;   // break down
            }
        }
        else
        {
            _poll_curr_rqs = DevRqsCode::class0_all;   // break down
        }
    }

    /* Poll class 1 */
    if (_poll_curr_rqs == DevRqsCode::class0_all)
    {
        if (_poll_class1_enabled)
        {
            if (_application_loader->read_class_data(1, false))
            {
                _poll_curr_rqs = DevRqsCode::class1_all;
                _dev_oper_status = DevOpStatusCode::poll_waiting;
                _poll_count++;
            }
            return;
        }
        else
        {
            _poll_curr_rqs = DevRqsCode::class1_all;   // break down
        }
    }

    /* Poll class 2 */
    if (_poll_curr_rqs == DevRqsCode::class1_all)
    {
        if (_poll_class2_enabled)
        {
            if (_application_loader->read_class_data(2, false))
            {
                _poll_curr_rqs = DevRqsCode::class2_all;
                _dev_oper_status = DevOpStatusCode::poll_waiting;
                _poll_count++;
            }
            return;
        }
        else
        {
            _poll_curr_rqs = DevRqsCode::class2_all;   // break down
        }
    }

    /* Poll class 3 */
    if (_poll_curr_rqs == DevRqsCode::class2_all)
    {
        if (_poll_class3_enabled)
        {
            if (_application_loader->read_class_data(3, false))
            {
                _poll_curr_rqs = DevRqsCode::class3_all;
                _dev_oper_status = DevOpStatusCode::poll_waiting;
                _poll_count++;
            }
            return;
        }
        else
        {
            _poll_curr_rqs = DevRqsCode::class3_all;   // break down
        }
    }

    /* Stop poll */
    if (_poll_curr_rqs == DevRqsCode::class3_all)
    {
        stop();
        return;
    }
}

void Device::proc_cmd()
{
    if (_cmd_point_list.empty())
    {
        stop();
        return;
    }

    const Dnp3Point& __point = _cmd_point_list.front();

    switch (__point.type_id)
    {
    case PointTypeCode::bocmd:
        exec_cmd_crob();
        break;

    case PointTypeCode::ao16cmd:
        exec_cmd_ao();
        break;

    case PointTypeCode::ao32cmd:
        exec_cmd_ao();
        break;

    default:
        break;
    }

}

void Device::proc_timesync()
{
    if (_dev_oper_status == DevOpStatusCode::time_sync)
    {
        _application_loader->delay_measurement(true, false);
        _dev_oper_status = DevOpStatusCode::time_s1_waiting;
    }
    else if (_dev_oper_status == DevOpStatusCode::time_s1_waiting)
    {
        long long int __t_send = _datalink_loader->get_last_send_time();
        long long int __t_rcv = _datalink_loader->get_last_rcv_time();
        long long int __diff = __t_rcv - __t_send;
        unsigned long long __t_delay = _application_loader->get_time_delay();
        unsigned long long __tset = 0;
        timeb __utc_now;
        ftime(&__utc_now);
        __tset = (unsigned long long)((long long int)__utc_now.time * 1000
                                      + __utc_now.millitm);

        __tset = __tset + (__diff - __t_delay)/2;
        if (_debug)
            *_debug_stream  << "Write time: "
                            << timestamp_to_str(__tset)
                            << std::endl;
        _application_loader->write_abstime(__tset, true, false);
        _dev_oper_status = DevOpStatusCode::time_s2_waiting;
    }
    else
    {
        _dev_oper_status = DevOpStatusCode::poll_ready;
    }
}

void Device::exec_cmd_crob()
{
    Dnp3Point& __point = _cmd_point_list.front();

    if (_dev_oper_status == DevOpStatusCode::cmd_s1_waiting)
    {
        _application_loader->send_cmd_crob(&__point, 2, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s2_waiting;
        _cmd_point_list.pop();
        return;
    }

    switch (__point.cmd_type)
    {
    case CmdTypeCode::dona:
        _application_loader->send_cmd_crob(&__point, 4, false, false);
        _dev_oper_status = DevOpStatusCode::poll_ready;
        _cmd_point_list.pop(); // lay ra khoi queue, khong can quan tam toi
        break;

    case CmdTypeCode::dop:
        _application_loader->send_cmd_crob(&__point, 3, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s2_waiting;
        _cmd_point_list.pop(); // lay ra khoi queue, khong can quan tam toi
        break;

    case CmdTypeCode::sop:
        _application_loader->send_cmd_crob(&__point, 1, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s1_waiting;
        break;

    default:
        break;
    }

}

void Device::exec_cmd_ao()
{
    Dnp3Point& __point = _cmd_point_list.front();

    if (_dev_oper_status == DevOpStatusCode::cmd_s1_waiting)
    {
        _application_loader->send_cmd_ao(&__point, 2, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s2_waiting;
        _cmd_point_list.pop();
        return;
    }

    switch (__point.cmd_type)
    {
    case CmdTypeCode::dona:
        _application_loader->send_cmd_ao(&__point, 4, false, false);
        _dev_oper_status = DevOpStatusCode::poll_ready;
        _cmd_point_list.pop(); // lay ra khoi queue, khong can quan tam toi
        break;

    case CmdTypeCode::dop:
        _application_loader->send_cmd_ao(&__point, 3, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s2_waiting;
        _cmd_point_list.pop(); // lay ra khoi queue, khong can quan tam toi
        break;

    case CmdTypeCode::sop:
        _application_loader->send_cmd_ao(&__point, 1, true, false);
        _dev_oper_status = DevOpStatusCode::cmd_s1_waiting;
        break;

    default:
        break;
    }


}

int Device::check_timer()
{
    /*
    function nay duoc goi khi dang wait dieu gi do
    return 1: chua timeout
    return 0: thuc su timeout
    return -1: timer khong chay, day la gia tri khong mong doi => coding error
    */

    /* Check datalink timeout */
    if (_datalink_loader->timer_is_running())
    {
        if (_datalink_loader->is_timeout())
        {
            if (_debug) *_debug_stream << "Datalink is timeout...\n";
            _datalink_loader->act_on_timeout();
        }
        return 1;
    }

    /* Check application timeout */
    if (_application_loader->timer_is_running())
    {
        if (_application_loader->is_timeout())
        {
            if (_debug) *_debug_stream << "Application is timeout...\n";
            _application_loader->act_on_timeout();
            _dev_status = DevStatusCode::Bad;
            _startup_required = true;
            _class0_timer.stop();
            stop();
            return 0;
        }
        return 1;
    }

    switch (_dev_oper_status)
    {
    case DevOpStatusCode::cmd_s1_waiting:
    case DevOpStatusCode::cmd_s2_waiting:
        while (!_cmd_point_list.empty()) _cmd_point_list.pop();
        stop();
        break;
    default:
        stop();
        break;
    }
    if (_debug) *_debug_stream << "*o* Unexpected behaviour...\n";
    return -1;
}

void Device::check_IIN()
{
    _dev_oper_status = DevOpStatusCode::poll_ready;
    ApplicationResponseHeader __header = _application_loader->get_rsp_header();
    if (__header.IIN.class1_events())
    {
        // Khi vao proc_poll, se poll class 1
        _poll_curr_rqs = DevRqsCode::class0_all;
        if (_debug) *_debug_stream << "Class 1 data available...\n";
    }
    else if (__header.IIN.class2_events())
    {
        // Khi vao proc_poll, se poll class 2
        _poll_curr_rqs = DevRqsCode::class1_all;
        if (_debug) *_debug_stream << "Class 2 data available...\n";
    }
    else if (__header.IIN.class3_events())
    {
        // Khi vao proc_poll, se poll class 3
        _poll_curr_rqs = DevRqsCode::class2_all;
        if (_debug) *_debug_stream << "Class 3 data available...\n";
    }
    else if (__header.IIN.need_time())
    {
        if (_timesync_enabled) _dev_oper_status = DevOpStatusCode::time_sync;
        if (_debug) *_debug_stream << "Time sync request from outstation...\n";
    }
}

void Device::on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e)
{
    /*
    Application hoan toan co the raise event ma khong co du lieu ( day la truong hop null asdu)
    Device van can phai kiem tra trang thai IIN
    */

    _dev_status = DevStatusCode::Good;

    if (dnp3_point_rcv_event && (e->length > 0))
    {
        if (_debug) *_debug_stream << "[poll]Datapoints received...\n";
        dnp3_point_rcv_event->notify(this, e);
    }

    switch (_dev_oper_status)
    {
    case DevOpStatusCode::startup_waiting:
        _dev_oper_status = DevOpStatusCode::startup;
        break;
    case DevOpStatusCode::time_s2_waiting:
        _dev_oper_status = DevOpStatusCode::poll_ready;
        break;
    case DevOpStatusCode::poll_waiting:
        check_IIN();
        break;
    default:
        break;
    }
}

void Device::on_dnp3_point_uns_rcv_event(void* sender, Dnp3PointEventArg* e)
{
    /*
    Application hoan toan co the gui du lieu null asdu. Tuy nhien truong hop nay khong kiem tra IIN
    */

    if (dnp3_point_rcv_event && (e->length > 0))
    {
        if (_debug) *_debug_stream << "[uns]Datapoints received...\n";
        dnp3_point_rcv_event->notify(this, e);
    }

}

void Device::on_dnp3_cmd_echo_event(void* sender, Dnp3PointEventArg* e)
{
    if (_debug)
    {
        Dnp3PointVector& __points = *(e->points);
        Dnp3PointVector::size_type i;
        *_debug_stream << "Command feedback received...\n";
        for (i = 0; i < e->length; i++)
        {
            *_debug_stream
                    << std::dec
                    << "Type: "         << PointTypeCode::to_string(__points[i].type_id)
                    << ", Index: "      << __points[i].index
                    << ", Value: "      << __points[i].value
                    << ", Cmd Type: "   << __points[i].cmd_type
                    << ", TCC: "        << CmdTrClsCode::to_string(__points[i].TCC)
                    << ", Op Type: "    << CmdOperTypeCode::to_string(__points[i].op_type)
                    << ", Count: "      << (int)(__points[i].cmd_count)
                    << ", On Time: "    << __points[i].cmd_ontime
                    << ", Off Time: "   << __points[i].cmd_offtime
                    << ", Cmd Status: " << CmdStatusCode::to_string(__points[i].cmd_status)
                    << "\n";
        }
    }
    switch (_dev_oper_status)
    {
    case DevOpStatusCode::cmd_s1_waiting:
    case DevOpStatusCode::cmd_s2_waiting:
        proc_cmd();
        break;
    default:
        break;
    }
}

void Device::on_dnp3_time_delay_event(void* sender, Dnp3PointEventArg* e)
{
    if (_debug) *_debug_stream << "Time delay value received...\n";
    if (_dev_oper_status == DevOpStatusCode::time_s1_waiting)
    {
        proc_timesync();
    }
}


} // Dnp3Master namespace

