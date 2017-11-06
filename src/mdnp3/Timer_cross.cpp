#include "mdnp3/pch.h"

#ifndef LINUX_ONLY

#include "mdnp3/Timer_cross.h"

namespace Dnp3Master
{
using namespace std::chrono;

Timer_cross::Timer_cross()
{
    _tset = 0;
    _timer_is_running = false;
}


Timer_cross::~Timer_cross()
{
}


void Timer_cross::start()
{
    _tbegin = system_clock::now();
    _timer_is_running = true;
}


void Timer_cross::stop()
{
    _timer_is_running = false;
}


long Timer_cross::get_elapsed()
{
    if (_timer_is_running)
    {
        _tcurrent = system_clock::now();
        return duration_cast<milliseconds>(_tcurrent - _tbegin).count();
    }

    return _tset;
}


bool Timer_cross::is_timeout()
{
    return _tset < get_elapsed();
}

bool Timer_cross::is_running()
{
    return _timer_is_running;
}


} // Dnp3Master namespace

#endif // LINUX_ONLY
