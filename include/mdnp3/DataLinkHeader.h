#ifndef DATALINKHEADER_H_INCLUDED
#define DATALINKHEADER_H_INCLUDED

#include "mdnp3/DataLinkControlField.h"

namespace Dnp3Master
{
class DataLinkHeader
{
public:
    DataLinkHeader();

    virtual ~DataLinkHeader();

    void set_header(byte frame[]);

    unsigned short get_start()
    {
        return _start;
    }
    unsigned short get_length()
    {
        return _length;
    }
    unsigned short get_dest()
    {
        return _dest;
    }
    unsigned short get_source()
    {
        return _src;
    }
    DataLinkControlField get_control()
    {
        return _control;
    }

private:
    unsigned short _start;
    unsigned short _length;
    unsigned short _dest;
    unsigned short _src;
    DataLinkControlField _control;
};
}

#endif // DATALINKHEADER_H_INCLUDED

#include "DataLinkControlField.h"



