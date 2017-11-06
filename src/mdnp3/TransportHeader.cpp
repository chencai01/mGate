#include "mdnp3/pch.h"
#include "mdnp3/TransportHeader.h"

namespace Dnp3Master
{

TransportHeader::TransportHeader()
{
    _header_byte = 0x00;
}


TransportHeader::TransportHeader(byte byt)
{
    _header_byte = byt;
}

TransportHeader::TransportHeader(const TransportHeader& obj)
{
    _header_byte = obj._header_byte;
}

const byte& TransportHeader::operator=(const byte& byt)
{
    _header_byte = byt;
    return byt;
}


TransportHeader& TransportHeader::operator=(const TransportHeader& obj)
{
    if (this != &obj)
    {
        _header_byte = obj._header_byte;
    }
    return *this;
}

TransportHeader::~TransportHeader()
{
}

} // Dnp3Master namespace

