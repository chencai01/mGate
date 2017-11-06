#ifndef SERIALCHANNEL_CROSS_H
#define SERIALCHANNEL_CROSS_H

#include "mdnp3/SerialChannelBase.h"
#include "thread"
#include "boost/asio/serial_port.hpp"
#include "mdnp3/Device.h"

namespace Dnp3Master
{

using namespace boost;

class SerialChannel_cross : public SerialChannelBase
{
public:
    SerialChannel_cross();
    virtual ~SerialChannel_cross();

    bool port_is_open()
    {
        return _port->is_open();
    }

    Device* get_device()
    {
        return _device;
    }

    bool is_running()
    {
        return _is_running;
    }

    void start(); // Khoi dong kenh bat dau lam viec
    void stop(); // Dung kenh
    void send_data(std::string data);

protected:

private:
    std::mutex _mtx_lck;
    std::thread _running_thread;
    std::thread _rcv_data_thread;

    asio::io_service _io;
    asio::serial_port* _port;
    Device* _device;


    boost::system::error_code _rcv_err;
    boost::system::error_code _send_err;

    typedef EventHandler<SerialChannel_cross, void, Dnp3PointEventArg> Dnp3PointEventHandler;
    Dnp3PointEventHandler* _dnp3_point_rcv_handler;
    void on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e);

    typedef EventHandler<SerialChannel_cross, void, FrameDataEventArg> FrameEventHandler;
    FrameEventHandler* _send_frame_handler;
    void on_send_frame_event(void* sender, FrameDataEventArg* e);

    /*********** PROCESSING FUNCTIONS ***********/

    void make_connection();
    void reset_connection();
    void rcv_callback(const system::error_code& error, std::size_t bytes_transferred);
    void send_callback(const system::error_code& error, std::size_t bytes_transferred);
    void rcv_threading(); // duoc khoi dong trong thread _rcv_data_thread
    void run_threading(); // duoc khoi dong trong thread _run_thread
    void poll_device();

    void check_buffer();
    void proc_frame_idle();
    void proc_frame_framming();
    void push_to_full_frame();
    void notify_frame();

    void check_send_data();
    void command_parser(std::string& cmd_str);

    /********************************************/

    void set_port_para();

};

} // Dnp3Master namespace

#endif // SERIALCHANNEL_CROSS_H
