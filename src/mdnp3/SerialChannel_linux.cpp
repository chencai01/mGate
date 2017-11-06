#include "mdnp3/pch.h"
#include "mdnp3/linux_only/SerialChannel_linux.h"
#include <string.h> // for strerror()
#include "mdnp3/Utility.h"

namespace Dnp3Master
{

SerialChannel_linux::SerialChannel_linux() : SerialChannelBase()
{
    //ctor
    _fd_port = -1;
    _device = new Device();
    _err_value = 0;

    _dnp3_point_rcv_handler = new Dnp3PointEventHandler();
    _dnp3_point_rcv_handler->connect(this, &SerialChannel_linux::on_dnp3_point_rcv_event);
    _device->dnp3_point_rcv_event = _dnp3_point_rcv_handler;

    _send_frame_handler = new FrameEventHandler();
    _send_frame_handler->connect(this, &SerialChannel_linux::on_send_frame_event);
    _device->get_datalink_loader()->send_frame_event = _send_frame_handler;

    update_to_iec104 = NULL;
    debug_callback = NULL;
    _device->set_debug_stream(_debug_stream);
    pthread_mutex_init(&_mtx_lck, NULL);
}

SerialChannel_linux::~SerialChannel_linux()
{
    //dtor
    _device->dnp3_point_rcv_event = NULL;
    _dnp3_point_rcv_handler->disconnect();

    _device->get_datalink_loader()->send_frame_event = NULL;
    _send_frame_handler->disconnect();

    delete _dnp3_point_rcv_handler;
    delete _send_frame_handler;
    delete _device;
    pthread_mutex_destroy(&_mtx_lck);
}

void* SerialChannel_linux::run_thread_functor(void* obj)
{
    SerialChannel_linux* __obj = (SerialChannel_linux*)obj;
    __obj->run_threading();
    return NULL;
}

void* SerialChannel_linux::rcv_thread_functor(void* obj)
{
    SerialChannel_linux* __obj = (SerialChannel_linux*)obj;
    __obj->rcv_threading();
    return NULL;
}

void SerialChannel_linux::run_threading()
{
    if (_fd_port == -1) init_port(false); // Khoi tao lan dau
    while (_is_running)
    {
        /* Check error */
        if (_err_value != 0)
        {
            std::cerr << "[Dnp3]Error code: " << _err_value << ", "
                      << strerror(_err_value) << std::endl;
            init_port(true);
        }
        else
        {
            if (_send_data_required) proc_send_data();
            poll_device();  // Poll device
            check_buffer(); // Kiem tra buffer

            std::string __debug_str = _debug_stream.str();
            if (!__debug_str.empty())
            {
                if (debug_callback != NULL)
                {
                    debug_callback(__debug_str);
                }
                _debug_stream.str("");
            }
        }
        usleep(50000); // sleep 50ms, usleep use unit microseconds
    }

    if (_fd_port != -1) close_port();
    if (_is_receving) _is_receving = false;
}

void SerialChannel_linux::rcv_threading()
{
    /* Khong su dung _debug_stream tai day
        vi function nay chay o thread khac
    */
    ssize_t __rcv_bytes = 0;
    while (_is_receving)
    {
        __rcv_bytes = read(_fd_port, _data_buffer, MAXFRAME);
        if (__rcv_bytes < 0)
        {
            _err_value = errno;
            std::cerr << "[Dnp3]read() error!\n";
            break;
        }
        else if (__rcv_bytes > 0)
        {
            pthread_mutex_lock(&_mtx_lck);
            for (ssize_t i = 0; i < __rcv_bytes; i++)
                _byte_list.push_back(_data_buffer[i]);
            pthread_mutex_unlock(&_mtx_lck);
        }
        usleep(10000); // sleep 10ms
    }
}

void SerialChannel_linux::start()
{
    if (_is_running) return;

    _is_running = true;
    _bytes_total = 0;
    _byte_list.clear();

    pthread_create(&_running_thread, NULL,
                   &SerialChannel_linux::run_thread_functor,
                   (void*)this);
    std::cout << "[Dnp3] started...\n";
}

void SerialChannel_linux::stop()
{
    if (!_is_running) return;

    _is_running = false;
    _is_receving = false;
    pthread_join(_running_thread, NULL);
    pthread_join(_rcv_data_thread, NULL);
    _device->deep_stop();
    _byte_list.clear();
    _bytes_total = 0;
    _rcv_frame_stt = FrameProcessStatus::idle;

    std::cout << "[Dnp3] stopped...\n";
}

void SerialChannel_linux::add_pfunc_callback(update_iec pfunc)
{
    update_to_iec104 = pfunc;
}

void SerialChannel_linux::add_debug_callback(debug_functor pfunc)
{
    debug_callback = pfunc;
}

void SerialChannel_linux::init_port(bool is_reset)
{
    _err_value = 0;
    _bytes_total = 0;
    _send_data_required = false;
    while (!_send_data_queue.empty())
    {
        _send_data_queue.pop();
    }
    _rcv_frame_stt = FrameProcessStatus::idle;
    _byte_list.clear();

    if (is_reset)
    {
        close_port();
        if (_is_receving)
        {
            _is_receving = false;
            pthread_join(_rcv_data_thread, NULL);
        }
    }

    while (_fd_port == -1)
    {
        sleep(1);
        if (!_is_running) return;
        open_port();
    }

    set_port_para();
    if (_err_value != 0) return; // error when set_port_para

    // Khoi tao tien trinh nhan du lieu
    if (!_is_receving)
    {
        _is_receving = true;
        pthread_create(&_rcv_data_thread, NULL,
                       &SerialChannel_linux::rcv_thread_functor,
                       (void*)this);
    }
}

void SerialChannel_linux::open_port()
{
    _fd_port = -1;
    _err_value = 0;
    _fd_port = open(_port_name.c_str(), O_RDWR | O_NOCTTY);
    if (_fd_port == -1)
    {
        _err_value = errno;
        std::cerr << "[DNP3] open_port() error!\n";
    }
}

void SerialChannel_linux::close_port()
{
    int __r = close(_fd_port);
    if (__r == -1)
    {
        _err_value = errno;
        std::cerr << "[DNP3]close_port() error!\n";
    }
    _fd_port = -1;
}

void SerialChannel_linux::set_port_para()
{
    termios __opt;
    if(tcgetattr(_fd_port, &__opt) < 0)
    {
        _err_value = errno;
        std::cerr << "[DNP3] tcgetattr() error!\n";
        return;
    }
    bzero(&__opt, sizeof(__opt));

    /* Baudrate */
    cfsetspeed(&__opt, baudrate(_baudrate));


    /* Character size */
    __opt.c_cflag &= ~CSIZE;
    switch (_char_size)
    {
    case CharacterSizeType::five:
        __opt.c_cflag |= CS5;
        break;
    case CharacterSizeType::six:
        __opt.c_cflag |= CS6;
        break;
    case CharacterSizeType::seven:
        __opt.c_cflag |= CS7;
        break;
    case CharacterSizeType::eight:
        __opt.c_cflag |= CS8;
        break;
    default:
        __opt.c_cflag |= CS8;
        break;
    }

    /* Parity bits */
    switch (_parity)
    {
    case ParityType::none:
        __opt.c_iflag |= IGNPAR;
        __opt.c_cflag &= ~PARENB;
        break;
    case ParityType::even:
        __opt.c_cflag |= PARENB;
        __opt.c_cflag &= ~PARODD;
        //__opt.c_iflag |= (INPCK /*| ISTRIP*/);
        break;
    case ParityType::odd:
        __opt.c_cflag |= PARENB;
        __opt.c_cflag |= PARODD;
        //__opt.c_iflag |= (INPCK /*| ISTRIP*/);
        break;
    default:
        __opt.c_iflag |= IGNPAR;
        __opt.c_cflag &= ~PARENB;
        break;
    }

    /* Stop bits*/
    switch (_stop_bit)
    {
    case StopBitsType::two:
        __opt.c_cflag |= CSTOPB;
        break;
    case StopBitsType::one:
    case StopBitsType::onepointfive:
        __opt.c_cflag &= ~CSTOPB;
        break;
    default:
        __opt.c_cflag &= ~CSTOPB;
        break;
    }

    /* Flow control */
    switch (_flow_control)
    {
    case FlowControlType::software:
        __opt.c_cflag &= ~CRTSCTS;
        __opt.c_iflag |= (IXON | IXOFF | IXANY);
        break;
    case FlowControlType::hardware:
        __opt.c_cflag |= CRTSCTS;
        __opt.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    case FlowControlType::none:
        __opt.c_cflag &= ~CRTSCTS;
        __opt.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    default:
        __opt.c_cflag &= ~CRTSCTS;
        __opt.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
    }

    /* Other options */
    __opt.c_cflag |= (CLOCAL | CREAD | HUPCL);

    __opt.c_cc[VMIN] = 0;
    __opt.c_cc[VTIME] = 1;

    //fcntl(_fd_port, F_SETFL, 0);
    if (tcsetattr(_fd_port, TCSANOW, &__opt) < 0)
    {
        _err_value = errno;
        std::cerr << "[DNP3] tcsetattr() error!\n";
        return;
    }
    tcflush(_fd_port, TCIFLUSH); // clean rcv
}

speed_t SerialChannel_linux::baudrate(BaudrateType speed)
{
    speed_t __rtn = B9600;
    switch (speed)
    {
    case 0:
        __rtn = B0;
        break;
    case 50:
        __rtn = B50;
        break;
    case 75:
        __rtn = B75;
        break;
    case 110:
        __rtn = B110;
        break;
    case 134:
        __rtn = B134;
        break;
    case 150:
        __rtn = B150;
        break;
    case 200:
        __rtn = B200;
        break;
    case 300:
        __rtn = B300;
        break;
    case 600:
        __rtn = B600;
        break;
    case 1200:
        __rtn = B1200;
        break;
    case 4800:
        __rtn = B4800;
        break;
    case 9600:
        __rtn = B9600;
        break;
    case 19200:
        __rtn = B19200;
        break;
    case 38400:
        __rtn = B38400;
        break;
    case 57600:
        __rtn = B57600;
        break;
    case 115200:
        __rtn = B115200;
        break;
    default:
        break;
    }
    return __rtn;
}

void SerialChannel_linux::poll_device()
{
    using namespace std;
    static DevStatusCode::DevStatusCode __devStt = DevStatusCode::Bad;
    _device->start();
    if (_device->get_status() != __devStt)
    {
        __devStt = _device->get_status();
        if (__devStt == DevStatusCode::Bad)
        {
            if (update_to_iec104 == NULL) return;
            Dnp3PointMap::iterator __it = _point_list.begin();
            while (__it != _point_list.end())
            {
                (__it->second).flags = 0x00;
                update_to_iec104((__it->second).get_name(),
                                 (__it->second).value,
                                 convert_dnp3F_to_iecQ((__it->second).flags)
                                );
                if (_debug) _debug_stream   << "[DNP3] =>iec: "
                                                << (__it->second).to_string()
                                                << endl;
                __it++;
            }
        }
    }
}

void SerialChannel_linux::check_buffer()
{
    switch (_rcv_frame_stt)
    {
    case FrameProcessStatus::idle:
        proc_frame_idle();
        break;
    case FrameProcessStatus::framming:
        proc_frame_framming();
        break;
    }
}

void SerialChannel_linux::proc_frame_idle()
{
    if (_byte_list.size() < 3) return;

    _bytes_total = 0;
    ByteList::iterator __it = _byte_list.begin();
    byte __start_1 = *(__it++);
    byte __start_2 = *(__it++);
    int __len = (int)(*__it);

    _bytes_total = (unsigned int)calc_bytes_from_len(__len);

    // Check frame header
    // Neu da co loi header thi phai xoa toan bo buffer luon
    if ((__start_1 != 0x05) || (__start_2 != 0x64) ||
        (__len < 5)         || (_bytes_total > MAXFRAME))
    {
        if (_debug)
        {
            _debug_stream << "[Dnp3] Frame error:";
            _debug_stream << std::hex << std::setfill('0');
            ByteList::iterator __it = _byte_list.begin();
            while (__it != _byte_list.end())
            {
                _debug_stream << " " << std::setw(2) << (int)(*__it);
                __it++;
            }
            _debug_stream << "\n";
        }
        pthread_mutex_lock(&_mtx_lck);
        _byte_list.clear();
        pthread_mutex_unlock(&_mtx_lck);
        return;
    }


    // Xu ly luon neu co the
    if (_byte_list.size() >= _bytes_total)
    {
        push_to_full_frame();
        if(_debug)
        {
            if (check_frame(_full_frame, _bytes_total, _debug_stream))
                notify_frame();
        }
        else
        {
            if (check_frame(_full_frame, _bytes_total))
                notify_frame();
        }
        _bytes_total = 0; // always reset
    }
    else
    {
        _rcv_frame_stt = FrameProcessStatus::framming;
    }
}

void SerialChannel_linux::proc_frame_framming()
{
    if (_byte_list.size() >= _bytes_total)
    {
        push_to_full_frame();
        if(_debug)
        {
            if (check_frame(_full_frame, _bytes_total, _debug_stream))
                notify_frame();
        }
        else
        {
            if (check_frame(_full_frame, _bytes_total))
                notify_frame();
        }
        _rcv_frame_stt = FrameProcessStatus::idle;
        _bytes_total = 0;
    }
}

void SerialChannel_linux::push_to_full_frame()
{
    pthread_mutex_lock(&_mtx_lck);
    ByteList::iterator __it = _byte_list.begin();
    for (unsigned int i = 0; i < _bytes_total; i++)
    {
        _full_frame[i] = *__it;
        __it++;
    }
    _byte_list.erase(_byte_list.begin(), __it);
    pthread_mutex_unlock(&_mtx_lck);
}

void SerialChannel_linux::notify_frame()
{
    FrameDataEventArg e;
    e.frame = _full_frame;
    e.length = _bytes_total;
    _device->get_datalink_loader()->on_frame_received(this, &e);
}

void SerialChannel_linux::on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e)
{
    using namespace std;
    string __check_n;
    vector<Dnp3Point>& __points = *(e->points);
    vector<Dnp3Point>::size_type i;
    Dnp3PointMap::iterator __it;
    for (i = 0; i < __points.size(); i++)
    {
        __check_n = __points[i].get_name();
        __it = _point_list.find(__check_n);
        if (__it != _point_list.end()) // Da co trong danh sach
        {
            if ((__it->second).update(__points[i]))
            {
                if (update_to_iec104 != NULL)
                {
                    update_to_iec104((__it->second).get_name(),
                                     (__it->second).value,
                                     convert_dnp3F_to_iecQ((__it->second).flags)
                                    );
                    if (_debug) _debug_stream   << "[DNP3] =>iec: "
                                                    << (__it->second).to_string()
                                                    << endl;
                }
            }
        }
        else // Chua co trong danh sach, them moi
        {
            _point_list[__check_n] = __points[i];
            if (update_to_iec104 != NULL)
            {
                update_to_iec104(__points[i].get_name(),
                                 __points[i].value,
                                 convert_dnp3F_to_iecQ(__points[i].flags)
                                );
                if (_debug) _debug_stream   <<"[DNP3] =>iec: "
                                                << __points[i].to_string()
                                                << endl;
            }
        }

    }
}

void SerialChannel_linux::on_send_frame_event(void* sender, FrameDataEventArg* e)
{
    ssize_t __rtn = 0;
    __rtn = write(_fd_port, e->frame, (size_t)e->length);

    if (__rtn < 0)
    {
        _err_value = errno;
        std::cerr << "[DNP3] write() error!\n";
    }
}


void SerialChannel_linux::send_data(std::string data)
{
    if (data == "dc1")
    {
        set_debug(true);
    }
    else if (data == "dc0")
    {
        set_debug(false);
    }
    else if (data == "dd1")
    {
        _device->set_ddev(true);
    }
    else if (data == "dd0")
    {
        _device->set_ddev(false);
    }
    else if (data == "dap1")
    {
        _device->set_dap(true);
    }
    else if (data == "dap0")
    {
        _device->set_dap(false);
    }
    else if (data == "dtr1")
    {
        _device->set_dtr(true);
    }
    else if (data == "dtr0")
    {
        _device->set_dtr(false);
    }
    else if (data == "ddl1")
    {
        _device->set_ddl(true);
    }
    else if (data == "ddl0")
    {
        _device->set_ddl(false);
    }
    else if (data == "d0")
    {
        set_debug(false);
        _device->set_debug(false, false, false, false);
    }
    else
    {
        _send_data_queue.push(data);
        _send_data_required = true;
    }
}

void SerialChannel_linux::proc_send_data()
{
    if (_send_data_queue.empty())
    {
        _send_data_required = false;
        return;
    }

    using namespace std;
    vector<string> __token;
    string __s = _send_data_queue.front();
    _send_data_queue.pop();
    command_parser(__s);
}

void SerialChannel_linux::command_parser(std::string& cmd_str)
{
    /*
    type_id: bocmd, ao16cmd, ao32cmd
    fc: sop | dop | dona
    op_type: nul | pulseon | pulseoff | latchon | latchoff
    tcc: nul | close | trip | reserved
    count, on_time, off_time
    */

    using namespace std;
    vector<string> __token;
    split_string(__token, cmd_str, '='); // tag=value
    if (__token.size() < 2) return;
    string __tag = __token[0];
    string __value = __token[1];

    __token.clear();
    split_string(__token, __tag, '.'); // Phan tach truong lenh dieu khien

    // binary ouput
    if (__token[0] == "bocmd")
    {
        //[type_id].[index].[fc].[op_type].[tcc].count].[on_time].[off_time]=1
        if (__token.size() < 8) return;
        Dnp3Point __point;
        __point.type_id = PointTypeCode::bocmd;
        __point.index = str_to_num<unsigned int>(__token[1]);

        // fc: sop | dop | dona
        if      (__token[2] == "sop")       __point.cmd_type = CmdTypeCode::sop;
        else if (__token[2] == "dop")       __point.cmd_type = CmdTypeCode::dop;
        else if (__token[2] == "dona")      __point.cmd_type = CmdTypeCode::dona;
        else return;

        // op_type: nul | pulseon | pulseoff | latchon | latchoff
        if      (__token[3] == "pulseon")   __point.op_type = CmdOperTypeCode::pulseon;
        else if (__token[3] == "pulseoff")  __point.op_type = CmdOperTypeCode::pulseoff;
        else if (__token[3] == "latchon")   __point.op_type = CmdOperTypeCode::latchon;
        else if (__token[3] == "latchoff")  __point.op_type = CmdOperTypeCode::latchoff;
        else if (__token[3] == "nul")       __point.op_type = CmdOperTypeCode::nul;
        else return;

        // tcc: nul | close | trip | reserved
        if      (__token[4] == "close")     __point.TCC = CmdTrClsCode::close;
        else if (__token[4] == "trip")      __point.TCC = CmdTrClsCode::trip;
        else if (__token[4] == "nul")       __point.TCC = CmdTrClsCode::nul;
        else return;

        __point.cmd_count = str_to_num<int>(__token[5]); // cast to integer first
        __point.cmd_ontime = str_to_num<int>(__token[6]);
        __point.cmd_offtime = str_to_num<int>(__token[7]);
        __point.value = __value;

        _device->write_cmd_point(__point);
        return;
    }

    // analog output 16 bit
    if ((__token[0] == "ao16cmd") || (__token[0] == "ao32cmd"))
    {
        // type_id.index.fc=value
        if (__token.size() < 3) return;
        Dnp3Point __point;
        if (__token[0] == "ao16cmd") __point.type_id = PointTypeCode::ao16cmd;
        else __point.type_id = PointTypeCode::ao32cmd;
        __point.index = str_to_num<unsigned int>(__token[1]);

        // fc: sop | dop | dona
        if      (__token[2] == "sop")       __point.cmd_type = CmdTypeCode::sop;
        else if (__token[2] == "dop")       __point.cmd_type = CmdTypeCode::dop;
        else if (__token[2] == "dona")      __point.cmd_type = CmdTypeCode::dona;
        else return;

        __point.value = __value;

        _device->write_cmd_point(__point);
        return;
    }
}

} // Dnp3Master namespace
