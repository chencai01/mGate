#include "mdnp3/pch.h"
#include "mdnp3/DataLinkHeader.h"

namespace Dnp3Master
{

DataLinkHeader::DataLinkHeader()
{
    _start = 0;
    _length = 0;
    _dest = 0;
    _src = 0;
}


DataLinkHeader::~DataLinkHeader()
{
}

void DataLinkHeader::set_header(byte frame[])
{
    _start = (((unsigned short)frame[0] << 8) & 0xff00) | (unsigned short)frame[1];
    _length = (unsigned short)frame[2];
    _control = frame[3];
    _dest = (((unsigned short)frame[5] << 8) & 0xff00) | (unsigned short)frame[4];
    _src = (((unsigned short)frame[7] << 8) & 0xff00) | (unsigned short)frame[6];
}

} // Dnp3Master namespace
