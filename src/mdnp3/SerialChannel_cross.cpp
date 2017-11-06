#include "mdnp3/pch.h"

#ifndef LINUX_ONLY

#include "mdnp3/SerialChannel_cross.h"

namespace Dnp3Master
{

SerialChannel_cross::SerialChannel_cross() : SerialChannelBase()
{
    //ctor
    _port = new boost::asio::serial_port(_io);

    _rcv_err.clear();
    _send_err.clear();

    _device = new Device();

    _dnp3_point_rcv_handler = new Dnp3PointEventHandler();
    _dnp3_point_rcv_handler->connect(this, &SerialChannel_cross::on_dnp3_point_rcv_event);
    _device->dnp3_point_rcv_event = _dnp3_point_rcv_handler;

    _send_frame_handler = new FrameEventHandler();
    _send_frame_handler->connect(this, &SerialChannel_cross::on_send_frame_event);
    _device->get_datalink_loader()->send_frame_event = _send_frame_handler;

    _device->set_debug_stream(_debug_stream);
}

SerialChannel::~SerialChannel()
{
    //dtor
    _device->dnp3_point_rcv_event = NULL;
    _dnp3_point_rcv_handler->disconnect();

    _device->get_datalink_loader()->send_frame_event = NULL;
    _send_frame_handler->disconnect();

    delete _dnp3_point_rcv_handler;
    delete _send_frame_handler;
    delete _device;
    if (_port)
    {
        system::error_code __err;
        if (_port->is_open()) _port->close(__err);
        delete _port;
    }
}

void SerialChannel::make_connection()
{
    if (_is_running) return;
    system::error_code __err;

    __err.clear();
    std::chrono::seconds __t_delay {1};
    int __count = 0;
    while (__count < 10) // retry trong 10 lan
    {
        std::this_thread::sleep_for(__t_delay);
        __count++;
        _port->open(_port_name, __err);
        #ifdef DNP3_DEBUG
        std::cout << "[make_conn]: " << __err.message() << "\n";
        #endif // DNP3_DEBUG
        if (__err.value() == 0) break; // ok!
    }
}

void SerialChannel::reset_connection()
{
    if(!_is_running) return;

    while(!_send_data_queue.empty()) {_send_data_queue.pop();}

    boost::system::error_code __err;
    _port->cancel(__err);
    _port->close(__err); // try to close

    if (_rcv_data_thread.joinable()) _rcv_data_thread.join(); // wait terminate rcv thread

    // start retry
    std::chrono::seconds __t_retry{3};
    while (_is_running)
    {
        std::this_thread::sleep_for(__t_retry);
        __err.clear();
        _port->open(_port_name, __err);
        #ifdef DNP3_DEBUG
        std::cout << "[reset_conn]: " << __err.message() << "\n";
        #endif // DNP3_DEBUG
        if (_port->is_open())
        {
            if (_is_running)
            {
                _send_err.clear();
                _rcv_err.clear();
                set_port_para();
                _io.reset();
                _rcv_data_thread = std::thread(&SerialChannel::rcv_threading, this);
            }
            else { _port->close(__err); }
            break; // exit loop
        }
    }
}

void SerialChannel::start()
{
    if (_is_running) // Kenh van dang chay
        return;

    make_connection();
    if (_port->is_open()) set_port_para();
    else return;

    _bytes_total = 0;
    _send_data_required = false;
    while (!_send_data_queue.empty()) {_send_data_queue.pop();}
    _rcv_frame_stt = FrameProcessStatus::idle;
    _rcv_err.clear();
    _send_err.clear();
    _io.reset();

    _running_thread = std::thread(&SerialChannel::run_threading, this); // Start poll data cycle
    _rcv_data_thread = std::thread(&SerialChannel::rcv_threading, this); // Start read data cycle

}

void SerialChannel::stop()
{
    if (!_is_running) // Kenh khong chay
        return;

    _is_running = false;

    system::error_code __err;
    _port->cancel(__err);
    _port->close(__err);
    _io.stop();

    if (_rcv_data_thread.joinable()) _rcv_data_thread.join();
    if (_running_thread.joinable()) _running_thread.join();
    #ifdef DNP3_DEBUG
    std::cout << "[stop_conn]: " << __err.message() << "\n";
    #endif // DNP3_DEBUG
}

void SerialChannel::set_port_para()
{
    using namespace boost::asio;

    /* Baud rate */
    _port->set_option(serial_port_base::baud_rate(_baudrate));

    /* Flow control */
    _port->set_option(serial_port_base::flow_control(_flow_control));

    /* Parity bit*/
    _port->set_option(serial_port_base::parity(_parity));

    /* Stop bits*/
    _port->set_option(serial_port_base::stop_bits(_stop_bit));

    /* Character size */
    _port->set_option(serial_port_base::character_size(static_cast<unsigned int>(_char_size)));

}

void SerialChannel::run_threading()
{
    if (_is_running) return;
    _is_running = true;
    std::chrono::milliseconds __t_wait{2};
    #ifdef DNP3_DEBUG
    std::cout << "run thread is running!\n";
    #endif // DNP3_DEBUG
    while (_is_running)
    {
        if (_send_data_required) check_send_data();
        poll_device();
        check_buffer(); // Kiem tra buffer

        std::string __debug_str = _debug_stream.str();
        if (!__debug_str.empty()) std::cout << __debug_str;
        _debug_stream.str("");

        /* Kiem tra loi read va write */
        if ((_rcv_err.value() != 0) || (_send_err.value() != 0))
        {
            #ifdef DNP3_DEBUG
            std::cout << "Found error when receive or send data, info:\n";
            std::cout << "rcv_err: " << _rcv_err.value() << " -->" << _rcv_err.message() << "\n";
            std::cout << "send_err: " << _send_err.value() << " -->" << _send_err.message() << "\n";
            #endif // DNP3_DEBUG
            reset_connection();
        }

        std::this_thread::sleep_for(__t_wait);
    }

    #ifdef DNP3_DEBUG
    std::cout << "run thread is stopped!\n";
    #endif // DNP3_DEBUG
}

void SerialChannel::rcv_threading()
{
    if (_is_receving) return;
    _is_receving = true;
    #ifdef DNP3_DEBUG
    std::cout << "rcv thread is running!\n";
    #endif // DNP3_DEBUG

    _port->async_read_some(boost::asio::buffer(_data_buffer, MAXFRAME),
                            boost::bind(&SerialChannel::rcv_callback,
                                        this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                          );

    _io.run(); // block until read some data or error occurre
    _is_receving = false;

    #ifdef DNP3_DEBUG
    std::cout << "rcv thread is stopped!\n";
    #endif // DNP3_DEBUG

}

void SerialChannel::rcv_callback(const system::error_code& error, std::size_t bytes_transferred)
{
    _rcv_err = error;
    if (_rcv_err.value() != 0) return;

    // Lock to push data to buffer
    _mtx_lck.lock();
    for (unsigned int i = 0; i < bytes_transferred; i++)
        _byte_list.push_back(_data_buffer[i]);
    _mtx_lck.unlock();

    // Continue receive data, avoid io.run() return in rcv_threading
    _port->async_read_some(boost::asio::buffer(_data_buffer, MAXFRAME),
                            boost::bind(&SerialChannel::rcv_callback,
                                        this,
                                        boost::asio::placeholders::error,
                                        boost::asio::placeholders::bytes_transferred)
                          );
}

void SerialChannel::send_data(std::string data)
{

    _send_data_queue.push(data);
    _send_data_required = true;

}

void SerialChannel::check_send_data()
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
    split_string(__token, __s, ':'); //command:info (info -> tag=value)
    if (__token.size() > 1)
    {
        if (__token[0] == "write") command_parser(__token[1]);
    }

}

void SerialChannel::command_parser(std::string& cmd_str)
{
    /*
    type_id: bocmd, ao16cmd, ao32cmd
    fc: sop | dop | dona
    op_type: nul | pulseon | pulseoff | latchon | latchoff
    tcc: nul | close | trip | reserved
    count, on_time, off_time
    write:type_id.index.fc.op_type.tcc.count.on_time.off_time=1 // bocmd
    write:type_id.index.fc=value //aocmd

    */

    using namespace std;
    vector<string> __token;
    split_string(__token, cmd_str, '='); // tag=value
    if (__token.size() < 2) return;
    string __tag = __token[0];
    string __value = __token[1];

    __token.clear();
    split_string(__token, __tag, '.');
    // binary ouput
    if (__token[0] == "bocmd")
    {
        if (__token.size() < 8) return;
        Dnp3Point __point;
        __point.type_id = PointTypeCode::bocmd;
        __point.index = stoul(__token[1]);

        // fc: sop | dop | dona
        if      (__token[2] == "sop")       __point.cmd_type = CmdTypeCode::sop;
        else if (__token[2] == "dop")       __point.cmd_type = CmdTypeCode::dop;
        else                                __point.cmd_type = CmdTypeCode::dona;

        // op_type: nul | pulseon | pulseoff | latchon | latchoff
        if      (__token[3] == "pulseon")   __point.op_type = CmdOperTypeCode::pulseon;
        else if (__token[3] == "pulseoff")  __point.op_type = CmdOperTypeCode::pulseoff;
        else if (__token[3] == "latchon")   __point.op_type = CmdOperTypeCode::latchon;
        else if (__token[3] == "latchoff")  __point.op_type = CmdOperTypeCode::latchoff;
        else                                __point.op_type = CmdOperTypeCode::nul;

        // tcc: nul | close | trip | reserved
        if      (__token[4] == "close")     __point.TCC = CmdTrClsCode::close;
        else if (__token[4] == "trip")      __point.TCC = CmdTrClsCode::trip;
        else                                __point.TCC = CmdTrClsCode::nul;

        __point.cmd_count = (byte)stoul(__token[5]);
        __point.cmd_ontime = stoul(__token[6]);
        __point.cmd_offtime = stoul(__token[7]);
        __point.value = __value;

        _device->write_cmd_point(__point);
        return;
    }

    // analog output 16 bit
    if (__token[0] == "ao16cmd")
    {
        if (__token.size() < 3) return;
        Dnp3Point __point;
        __point.type_id = PointTypeCode::ao16cmd;
        __point.index = stoul(__token[1]);

        // fc: sop | dop | dona
        if      (__token[2] == "sop")       __point.cmd_type = CmdTypeCode::sop;
        else if (__token[2] == "dop")       __point.cmd_type = CmdTypeCode::dop;
        else                                __point.cmd_type = CmdTypeCode::dona;

        __point.value = __value;

        _device->write_cmd_point(__point);
        return;
    }

    // analog output 32 bit
    if (__token[0] == "ao32cmd")
    {
        if (__token.size() < 3) return;
        Dnp3Point __point;
        __point.type_id = PointTypeCode::ao32cmd;
        __point.index = stoul(__token[1]);
        __point.value = __value;

        // fc: sop | dop | dona
        if      (__token[2] == "sop")       __point.cmd_type = CmdTypeCode::sop;
        else if (__token[2] == "dop")       __point.cmd_type = CmdTypeCode::dop;
        else                                __point.cmd_type = CmdTypeCode::dona;

        _device->write_cmd_point(__point);
        return;
    }

}

void SerialChannel::check_buffer()
{
    switch (_rcv_frame_stt)
    {
        case FrameProcessStatus::idle: proc_frame_idle(); break;
        case FrameProcessStatus::framming: proc_frame_framming(); break;
    }
}

void SerialChannel::proc_frame_idle()
{
    if (_byte_list.size() < 10) return;

        _bytes_total = 0;
        ByteList::iterator __it = _byte_list.begin();
        byte __start_1 = *(__it++);
        byte __start_2 = *(__it++);
        unsigned int __len = (unsigned int)*(__it);

        // Check frame header
        if ((__start_1 != 0x05) || (__start_2 != 0x64) || (__len < 5))
        {
            _byte_list.pop_front(); _byte_list.pop_front(); _byte_list.pop_front();
            return;
        }

        // Calculate total of crc
        unsigned int __crc = 2 + 2 * ((__len - 5) / 16); // can than phep chia co du, phai de ((__len - 5) / 16) thi moi dung
        if ((__len - 5) % 16 != 0) __crc += 2;

        _bytes_total = 3 + __len + __crc;

        // Check max length
        if (_bytes_total > 292)
        {
            _byte_list.pop_front(); _byte_list.pop_front(); _byte_list.pop_front();
            return;
        }

        // Xu ly luon neu co the
        if (_byte_list.size() >= _bytes_total)
        {
            push_to_full_frame();
            if (check_frame(_full_frame, _bytes_total)) notify_frame();
        }
        else {_rcv_frame_stt = FrameProcessStatus::framming;}
}

void SerialChannel::proc_frame_framming()
{
    if (_byte_list.size() >= _bytes_total)
    {
        push_to_full_frame();
        if (check_frame(_full_frame, _bytes_total)) notify_frame();
        _rcv_frame_stt = FrameProcessStatus::idle;
    }
}

void SerialChannel::push_to_full_frame()
{
    _mtx_lck.lock();
    ByteList::iterator __it = _byte_list.begin();
    for (unsigned int i = 0; i < _bytes_total; i++)
    {
        _full_frame[i] = *__it;
        __it++;
    }
    _byte_list.erase(_byte_list.begin(), __it);
    _mtx_lck.unlock();
}

void SerialChannel::notify_frame()
{
    FrameDataEventArg e;
    e.frame = _full_frame;
    e.length = _bytes_total;
    _device->get_datalink_loader()->on_frame_received(this, &e);
}

void SerialChannel::poll_device()
{
    _device->start();
}

void SerialChannel::on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e)
{
    for (Dnp3Point& point : *(e->points))
    {
        std::cout << point.to_string() << "\n";
    }
}

void SerialChannel::on_send_frame_event(void* sender, FrameDataEventArg* e)
{
    _send_err.clear();
//    _port->async_write_some(boost::asio::buffer(e->frame, e->length),
//                        boost::bind(&SerialChannel::send_callback,
//                                    this,
//                                    boost::asio::placeholders::error,
//                                    boost::asio::placeholders::bytes_transferred)
//                      );
    boost::asio::write(*_port, boost::asio::buffer(e->frame, e->length), _send_err);
}

void SerialChannel::send_callback(const system::error_code& error, std::size_t bytes_transferred)
{
    _send_err = error;
}

} // Dnp3Master namespace

#endif // LINUX_ONLY
