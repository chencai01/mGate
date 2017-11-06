#include "mdnp3/pch.h"
#include "mdnp3/Dnp3Points.h"

namespace Dnp3Master
{
Dnp3Point::Dnp3Point()
{
    type_id = PointTypeCode::undefined;
    index = 0;
    flags = 0x00;
    value = "";
    timestamp = 0;

    cmd_type = CmdTypeCode::dop;
    op_type = CmdOperTypeCode::nul;
    TCC = CmdTrClsCode::nul;
    cmd_status = CmdStatusCode::SUCCESS;
    clear_bit = false;
    cmd_count = 1;
    cmd_ontime = 100;
    cmd_offtime = 0;
}

Dnp3Point::Dnp3Point(const Dnp3Point& obj)
{
    type_id = obj.type_id;
    index = obj.index;
    flags = obj.flags;
    value = obj.value;
    timestamp = obj.timestamp;

    cmd_type = obj.cmd_type;
    op_type = obj.op_type;
    TCC = obj.TCC;
    cmd_status = obj.cmd_status;
    clear_bit = obj.clear_bit;
    cmd_count = obj.cmd_count;
    cmd_ontime = obj.cmd_ontime;
    cmd_offtime = obj.cmd_offtime;
}

Dnp3Point::~Dnp3Point() {};

Dnp3Point& Dnp3Point::operator=(const Dnp3Point& obj)
{
    if (this != &obj)
    {
        type_id = obj.type_id;
        index = obj.index;
        flags = obj.flags;
        value = obj.value;
        timestamp = obj.timestamp;

        cmd_type = obj.cmd_type;
        op_type = obj.op_type;
        TCC = obj.TCC;
        cmd_status = obj.cmd_status;
        clear_bit = obj.clear_bit;
        cmd_count = obj.cmd_count;
        cmd_ontime = obj.cmd_ontime;
        cmd_offtime = obj.cmd_offtime;
    }

    return *this;
}

std::string Dnp3Point::get_name()
{
    std::string __name = PointTypeCode::to_string(type_id) + "." +
                         num_to_str<unsigned int>(index);
    return __name;
}

bool Dnp3Point::update(Dnp3Point& obj)
{
    if (this->type_id != obj.type_id) return false;
    if (this->index != obj.index) return false;
    bool __rtn = false;
    if (this->value != obj.value)
    {
        this->value = obj.value;
        __rtn = true;
    }
    if (this->flags != obj.flags)
    {
        this->flags = obj.flags;
        __rtn = true;
    }
    if (__rtn)
        if (this->timestamp != obj.timestamp)
            this->timestamp = obj.timestamp;

    return __rtn;
}

std::string Dnp3Point::to_string()
{
    std::string __str = "Type= "  + PointTypeCode::to_string(type_id) +
                        "\tIdx= " + num_to_str<unsigned int>(index) +
                        "\tValue= " + value +
                        "\tFlag= "  + PointTypeCode::flags_to_string(type_id, (int)flags) +
                        "\tTime= "  + timestamp_to_str(timestamp);
    return __str;
}

} // Dnp3Master namespace
