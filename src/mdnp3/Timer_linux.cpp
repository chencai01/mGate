#include "mdnp3/pch.h"
#include "mdnp3/linux_only/Timer_linux.h"

namespace Dnp3Master
{

Timer_linux::Timer_linux()
{
    _tset = 0;
    _timer_is_running = false;
}

Timer_linux::~Timer_linux()
{
}

void Timer_linux::start()
{
    ftime(&_tbegin);
    //clock_gettime(CLOCK_REALTIME, &_tbegin);
    //gettimeofday(&_tbegin, NULL);
    _timer_is_running = true;
}

void Timer_linux::stop()
{
    _timer_is_running = false;
}


long Timer_linux::get_elapsed()
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


bool Timer_linux::is_timeout()
{
    return _tset < get_elapsed();
}

bool Timer_linux::is_running()
{
    return _timer_is_running;
}

void Timer_linux::print_info()
{
    using namespace std;
    cout << "[" << _tset << "] "
         << "[" << get_elapsed() << "] "
         << "[" << _tbegin.time << ", " << _tbegin.millitm << "] "
         << "[" << _tcurrent.time << ", " << _tcurrent.millitm << "]\n";
}

} // Dnp3Master namespace
