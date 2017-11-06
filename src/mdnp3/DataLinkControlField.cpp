#include "mdnp3/pch.h"
#include "mdnp3/DataLinkControlField.h"
#include "mdnp3/Utility.h"

namespace Dnp3Master
{

//    RESET_LINK        = 0x0100,  RESET_USER_PROCESS  = 0x0101,  TEST_LINK           = 0x0102,
//    USERDATA_CONFIRM  = 0x0103,  USERDATA_NO_CONFIRM = 0x0104,  REQUEST_LINK_STATUS = 0x0109,
//    ACK               = 0x0000,  NACK                = 0x0001,  LINK_STATUS         = 0x0011,
//    LINK_NOT_FUNCTION = 0x0014,  LINK_NOT_USE        = 0x0015,  UNDEFINED           = 0xffff
std::string FunctionCode::to_string(int enum_code)
{
    using namespace FunctionCode;
    std::string __rtn;
    switch (enum_code)
    {
    case RESET_LINK:
        __rtn = "RESET_LINK";
        break;
    case RESET_USER_PROCESS:
        __rtn = "RESET_USER_PROCESS";
        break;
    case TEST_LINK:
        __rtn = "TEST_LINK";
        break;
    case USERDATA_CONFIRM:
        __rtn = "USERDATA_CONFIRM";
        break;
    case USERDATA_NO_CONFIRM:
        __rtn = "USERDATA_NO_CONFIRM";
        break;
    case REQUEST_LINK_STATUS:
        __rtn = "REQUEST_LINK_STATUS";
        break;
    case ACK:
        __rtn = "ACK";
        break;
    case NACK:
        __rtn = "NACK";
        break;
    case LINK_STATUS:
        __rtn = "LINK_STATUS";
        break;
    case LINK_NOT_FUNCTION:
        __rtn = "LINK_NOT_FUNCTION";
        break;
    case LINK_NOT_USE:
        __rtn = "LINK_NOT_USE";
        break;
    case UNDEFINED:
        __rtn = "UNDEFINED";
        break;
    default:
        __rtn = num_to_str(enum_code);
        break;
    }
    return __rtn;
}

DataLinkControlField::DataLinkControlField()
{
    _byte_val = 0xff;
    set_bits();
}


DataLinkControlField::DataLinkControlField(byte byte_val)
{
    _byte_val = byte_val;
    set_bits();
}


DataLinkControlField::DataLinkControlField(const DataLinkControlField& obj)
{
    _byte_val = obj._byte_val;
    set_bits();
}


const byte& DataLinkControlField::operator=(const byte& byte_val)
{
    _byte_val = byte_val;
    set_bits();
    return byte_val;
}


DataLinkControlField& DataLinkControlField::operator=(const DataLinkControlField& obj)
{
    if (this != &obj)
    {
        _byte_val = obj._byte_val;
        set_bits();
    }

    return *this;
}


DataLinkControlField::~DataLinkControlField()
{
}


void DataLinkControlField::set_bits()
{
    _dir = (_byte_val & 0x80) == 0x80;
    _prm = (_byte_val & 0x40) == 0x40;
    _fcb = (_byte_val & 0x20) == 0x20;
    _fcv_dfc = (_byte_val & 0x10) == 0x10;
    if (_prm)
        _fc = (FunctionCode::FunctionCode)((unsigned short)(_byte_val & 0x0f) | 0x0100);
    else
        _fc = (FunctionCode::FunctionCode)(_byte_val & 0x0f);
}

}
