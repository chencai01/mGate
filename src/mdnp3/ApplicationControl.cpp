#include "mdnp3/pch.h"
#include "mdnp3/ApplicationControl.h"


namespace Dnp3Master
{

ApplicationControl::ApplicationControl() : _ctrl_byte(0x00)
{
}

ApplicationControl::~ApplicationControl()
{
}

ApplicationControl::ApplicationControl(byte byt)
{
    _ctrl_byte = byt;
}


ApplicationControl::ApplicationControl(const ApplicationControl& obj)
{
    _ctrl_byte = obj._ctrl_byte;
}


const byte& ApplicationControl::operator=(const byte& byt)
{
    _ctrl_byte = byt;
    return _ctrl_byte;
}

ApplicationControl& ApplicationControl::operator=(const ApplicationControl& obj)
{
    if (this != &obj)
    {
        _ctrl_byte = obj._ctrl_byte;
    }

    return *this;
}


byte ApplicationControl::create_ap_control(bool fir, bool fin, bool con, bool uns, int seq)
{
    byte __ctrl = 0x00;

    if (fir) __ctrl |= 0x80;
    if (fin) __ctrl |= 0x40;
    if (con) __ctrl |= 0x20;
    if (uns) __ctrl |= 0x10;
    __ctrl |= (byte)seq & 0x0f;

    return __ctrl;
}


} // Dnp3Master namespace


