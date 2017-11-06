#ifndef SERIALCHANNEL_LINUX_H
#define SERIALCHANNEL_LINUX_H

#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include "mdnp3/SerialChannelBase.h"
//#include "Device.h"

namespace Dnp3Master
{

class SerialChannel_linux : public SerialChannelBase
{
public:
    typedef void (*update_iec)(std::string, std::string, char);
    typedef void (*debug_functor)(std::string);
    SerialChannel_linux();
    virtual ~SerialChannel_linux();

    virtual void start();
    virtual void stop();
    virtual void send_data(std::string data);
    Device* get_device()
    {
        return _device;
    }
    int dev_stt()
    {
        return (int)(_device->get_status());
    }
    void add_pfunc_callback(update_iec pfunc);
    void add_debug_callback(debug_functor pfunc);

protected:

private:
    pthread_t _running_thread;
    pthread_t _rcv_data_thread;
    pthread_mutex_t _mtx_lck;

    int _fd_port; // File descriptor for the port

    int _err_value;

    Device* _device;

    static void* run_thread_functor(void* obj); // func call run_threading
    static void* rcv_thread_functor(void* obj); // func call rcv_threading

    void run_threading();
    void rcv_threading();

    typedef EventHandler<SerialChannel_linux, void, Dnp3PointEventArg> Dnp3PointEventHandler;
    Dnp3PointEventHandler* _dnp3_point_rcv_handler;
    void on_dnp3_point_rcv_event(void* sender, Dnp3PointEventArg* e);

    typedef EventHandler<SerialChannel_linux, void, FrameDataEventArg> FrameEventHandler;
    FrameEventHandler* _send_frame_handler;
    void on_send_frame_event(void* sender, FrameDataEventArg* e);

    update_iec update_to_iec104;
    debug_functor debug_callback;

    void init_port(bool is_reset);
    void open_port();
    void close_port();
    void set_port_para();
    void poll_device();
    void check_buffer();
    void proc_frame_idle();
    void proc_frame_framming();
    void push_to_full_frame();
    void proc_send_data();
    void notify_frame();

    void command_parser(std::string& cmd_str);

    speed_t baudrate(BaudrateType speed);
};

} // Dnp3Master namespace

#endif // SERIALCHANNEL_LINUX_H
