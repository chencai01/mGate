#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <iostream>
#include <sys/timeb.h>

namespace S104{

class Timer
{
public:
    Timer();
    Timer(long t);
    ~Timer();
    void set_time(long t) {_tset = t*1000;}
    long get_time() {return _tset;}
    void start();
    void stop();
    long get_elapsed();
    bool is_timeout();
    bool is_running();

private:
    timeb _tbegin;
    timeb _tcurrent;
    long _tset;
    bool _timer_is_running;
};

}

#endif // TIMER_H_INCLUDED
