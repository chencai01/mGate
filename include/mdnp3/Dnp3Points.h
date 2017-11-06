#ifndef DNP3POINT_H_INCLUDED
#define DNP3POINT_H_INCLUDED

#include "mdnp3/Dnp3PointEnumerate.h"
#include "mdnp3/Utility.h"

namespace Dnp3Master
{

class Dnp3Point
{
public:
    Dnp3Point();
    Dnp3Point(const Dnp3Point& obj);
    virtual ~Dnp3Point();
    Dnp3Point& operator=(const Dnp3Point& obj);

    std::string get_name();
    bool update(Dnp3Point& obj);
    std::string to_string();

public:

    PointTypeCode::PointTypeCode            type_id;
    unsigned int                            index;
    byte                                    flags;
    std::string                             value;
    unsigned long long                      timestamp;
    CmdTypeCode::CmdTypeCode                cmd_type;
    CmdOperTypeCode::CmdOperTypeCode        op_type;
    CmdTrClsCode::CmdTrClsCode              TCC;
    CmdStatusCode::CmdStatusCode            cmd_status;
    bool                                    clear_bit;
    byte                                    cmd_count;
    unsigned int                            cmd_ontime;
    unsigned int                            cmd_offtime;
};

} // Dnp3Master namespace

#endif // DNP3POINT_H_INCLUDED


