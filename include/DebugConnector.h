#ifndef DEBUGCONNECTOR_H
#define DEBUGCONNECTOR_H

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#include <iostream>
#include <vector>

class DebugConnector
{
public:
    typedef void (*DebugCmdRcvFunctor)(std::string&);
    DebugConnector();
    virtual ~DebugConnector();

    void set_para(char* ip_addr, int port);
    void start();
    void stop();

    void add_cmd_rcv_callback(DebugCmdRcvFunctor pfunc);
    void send_debug_data(std::string& data, char head);

    bool is_connected()
    {
        return _is_connected;
    }

protected:

private:
    DebugCmdRcvFunctor _cmd_rcv_callback;

    pthread_t _run_thread;
    pthread_t _send_thread;
    pthread_mutex_t _mtx_lck;

    int _sockfd;
    int _port;
    std::string _ip_addr;
    sockaddr_in _serv_addr;
    char _rcv_buff[64];
    bool _is_running;
    bool _is_connected;
    std::string _send_buffer;


    int init();

    static void* run_thread_funtor(void* obj);
    static void* send_thread_functor(void* obj);
    void run_threading();
    void send_threading();

    ssize_t read_line(int fd, char* buffer, size_t n);

};

#endif // DEBUGCONNECTOR_H
