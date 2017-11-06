#include "mdnp3/pch.h"
#include "DebugConnector.h"
#include <iostream>

DebugConnector::DebugConnector()
{
    this->_sockfd = -1;
    this->_port = 4001;
    this->_ip_addr = "127.0.0.1";
    this->_is_running = false;
    this->_is_connected = false;
    this->_cmd_rcv_callback = NULL;
    pthread_mutex_init(&_mtx_lck, NULL);
}

DebugConnector::~DebugConnector()
{
    if (_is_connected) close(_sockfd);
    pthread_mutex_destroy(&_mtx_lck);
}


void DebugConnector::set_para(char* ip_addr, int port)
{
    this->_ip_addr = std::string(ip_addr);
    this->_port = port;
}

void DebugConnector::start()
{
    if (_is_running) return;
    _is_running = true;
    pthread_create(&_run_thread, NULL, &DebugConnector::run_thread_funtor,
                   (void*)this);
    pthread_create(&_send_thread, NULL, &DebugConnector::send_thread_functor,
                    (void*)this);
    std::cout << "[Debugger] started!\n";
}

void DebugConnector::stop()
{
    if (!_is_running) return;
    _is_running = false;
    shutdown(_sockfd, SHUT_RDWR);
    close(_sockfd);
    _is_connected = false;
    pthread_join(_run_thread, NULL);
    pthread_join(_send_thread, NULL);
    std::cout << "[Debugger] stopped!\n";
}

int DebugConnector::init()
{
    using namespace std;
    if (_sockfd < 0)
    {
        memset(_rcv_buff, '0', sizeof(_rcv_buff));
        memset(&_serv_addr, '0', sizeof(_serv_addr));
        _serv_addr.sin_port = htons(_port);
        _serv_addr.sin_family = AF_INET;
        if (inet_pton(AF_INET, _ip_addr.c_str(), &_serv_addr.sin_addr) <=0)
        {
            int __err = errno;
            cerr << "[Debugger] " << strerror(__err) << endl;
            return -1;
        }
        _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }

    if (_sockfd < 0 )
    {
        int __err = errno;
        cerr << "[Debugger] " << strerror(__err) << endl;
        return -1;
    }


    if (connect(_sockfd, (sockaddr*)&_serv_addr, sizeof(_serv_addr)) < 0)
    {
        return -1;
    }
    else cout << "[Debugger] connected!\n";
    return 1;
}

void* DebugConnector::run_thread_funtor(void* obj)
{
    DebugConnector* __debug = (DebugConnector*)obj;
    __debug->run_threading();
    return NULL;
}

void* DebugConnector::send_thread_functor(void* obj)
{
    DebugConnector* __debug = (DebugConnector*)obj;
    __debug->send_threading();
    return NULL;
}

void DebugConnector::run_threading()
{
    using namespace std;
    unsigned int __delay = 1;
    while (_is_running)
    {
        if (!_is_connected)
        {
            __delay = 10;
            if(init() > 0)
            {
                __delay = 1;
                _is_connected = true;
            }
        }

        if (_is_connected)
        {
            ssize_t __count = read_line(_sockfd, _rcv_buff, 64);
            if ( __count < 0)
            {
                shutdown(_sockfd, SHUT_RDWR);
                close(_sockfd);
                _sockfd = -1;
                _is_connected = false;
            }
            else
            {
                if (_cmd_rcv_callback != NULL)
                {
                    string __data(_rcv_buff);
                    _cmd_rcv_callback(__data);
                }
            }
        }
        sleep(__delay);
    }
}

void DebugConnector::send_threading()
{
    while(_is_connected)
    {
        if(_send_buffer.length() > 1)
        {
            pthread_mutex_lock(&_mtx_lck);
            send(_sockfd, _send_buffer.c_str(), _send_buffer.length(),0);
            _send_buffer.clear();
            pthread_mutex_unlock(&_mtx_lck);
        }
        usleep(100000);
    }
}

ssize_t DebugConnector::read_line(int fd, char* buffer, size_t n)
{
    ssize_t __count = 0;
    size_t __total = 0;
    char __ch;

    while (true)
    {
        __count = recv(fd, &__ch, 1, 0);
        if (__count <= 0) // error
        {
            return __count;
        }
        else // __count == 1
        {
            if (__ch == '\n') break;
            if ((__total < n-1) && (__ch != '\r'))
            {
                *buffer++ = __ch;
                __total++;
            }
        }
    }

    *buffer = '\0';
    return __total;
}

void DebugConnector::add_cmd_rcv_callback(DebugCmdRcvFunctor pfunc)
{
    _cmd_rcv_callback = pfunc;
}

void DebugConnector::send_debug_data(std::string& data, char head)
{
    pthread_mutex_lock(&_mtx_lck);

    if (_is_connected)
    {
        char __token = head;
        data.insert(data.begin(), __token);
        __token = 0x03;
        data.append(1, __token);
        _send_buffer.append(data);
        /*char __token = head;
        if (write(_sockfd, &__token, 1) > 0)
        {
            if (write(_sockfd, data.c_str(), data.length()) > 0)
            {
                __token = 0x03;
                write(_sockfd, &__token, 1);
            }
        }*/
    }
    pthread_mutex_unlock(&_mtx_lck);
}
