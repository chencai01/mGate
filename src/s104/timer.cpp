#include "s104/timer.h"

namespace S104{

Timer::Timer()
{
    _tset = 0;
    _timer_is_running = false;
}

Timer::Timer(long t){

    _tset = t * 1000;
    _timer_is_running = false;
}

Timer::~Timer()
{
}

void Timer::start()
{
    ftime(&_tbegin);
    _timer_is_running = true;
}

void Timer::stop()
{
    _timer_is_running = false;
}


long Timer::get_elapsed()
{
    if (_timer_is_running)
    {
        ftime(&_tcurrent);
        long __interval = _tcurrent.time - _tbegin.time;
        if (__interval < 0)
        {
            _tbegin = _tcurrent; // correct error
            return 0;
        }
        else if (__interval < 86400) // timer for one day only
        {
            long __msec = _tcurrent.millitm - _tbegin.millitm;
            __interval = __interval * 1000L + __msec;
            return __interval;
        }
        else
        {
            return __interval;
        }
    }
    return 0;
}


bool Timer::is_timeout()
{
    return _tset < get_elapsed();
}

bool Timer::is_running()
{
    return _timer_is_running;
}

}
