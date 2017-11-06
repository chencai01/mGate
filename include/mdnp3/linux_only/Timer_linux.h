#ifndef TIMER_LINUX_H
#define TIMER_LINUX_H

#include <sys/timeb.h>
//#include <sys/time.h>
//#include <time.h>

namespace Dnp3Master
{

class Timer_linux
{
public:
    Timer_linux();
    virtual ~Timer_linux();
    void set_time(long t)
    {
        _tset = t;
    }
    long get_time()
    {
        return _tset;
    }
    void start();
    void stop();
    long get_elapsed();
    bool is_timeout();
    bool is_running();

    void print_info();

protected:

private:
    //timespec _tbegin;
    //timespec _tcurrent;
    //timeval _tbegin;
    //timeval _tcurrent;
    timeb _tbegin;
    timeb _tcurrent;
    long _tset;
    bool _timer_is_running;
};

} // Dnp3Master namespace

#endif // TIMER_LINUX_H
