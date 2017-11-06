#ifndef TIMER_CROSS_H
#define TIMER_CROSS_H

#include <chrono>

class Timer_cross
{
public:
    Timer_cross();
    virtual ~Timer_cross();

    void set_time(long t) {_tset = t;}
    long get_time() {return _tset;}
    void start();
    void stop();
    long get_elapsed();
    bool is_timeout();
    bool is_running();
protected:

private:
    std::chrono::system_clock::time_point _tbegin;
    std::chrono::system_clock::time_point _tcurrent;
    long _tset;
    bool _timer_is_running;
};

#endif // TIMER_CROSS_H
