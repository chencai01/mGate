#include "mdnp3/pch.h"
#include "mdnp3/InternalIndication.h"
#include "mdnp3/Utility.h"

namespace Dnp3Master
{

//    BROADCAST      = 0x0001,      NO_FUNC_CODE_SUPPORT  = 0x0100,
//    CLASS_1_EVENTS = 0x0002,      OBJECT_UNKNOWN        = 0x0200,
//    CLASS_2_EVENTS = 0x0004,      PARAMETER_ERROR       = 0x0400,
//    CLASS_3_EVENTS = 0x0008,      EVENT_BUFFER_OVERFLOW = 0x0800,
//    NEED_TIME      = 0x0010,      ALREADY_EXECUTING     = 0x1000,
//    LOCAL_CONTROL  = 0x0020,      CONFIG_CORRUPT        = 0x2000,
//    DEVICE_TROUBLE = 0x0040,      RESERVED_2            = 0x4000,
//    DEVICE_RESTART = 0x0080,      RESERVED_1            = 0x8000
std::string IINFlags::to_string(int enum_code)
{
    using namespace IINFlags;
    std::string __rtn;
    if ((enum_code & BROADCAST) == BROADCAST)                         __rtn += "BROADCAST,";
    if ((enum_code & CLASS_1_EVENTS) == CLASS_1_EVENTS)               __rtn += "CLASS_1_EVENTS,";
    if ((enum_code & CLASS_2_EVENTS) == CLASS_2_EVENTS)               __rtn += "CLASS_2_EVENTS,";
    if ((enum_code & CLASS_3_EVENTS) == CLASS_3_EVENTS)               __rtn += "CLASS_3_EVENTS,";
    if ((enum_code & NEED_TIME) == NEED_TIME)                         __rtn += "NEED_TIME,";
    if ((enum_code & LOCAL_CONTROL) == LOCAL_CONTROL)                 __rtn += "LOCAL_CONTROL,";
    if ((enum_code & DEVICE_TROUBLE) == DEVICE_TROUBLE)               __rtn += "DEVICE_TROUBLE,";
    if ((enum_code & DEVICE_RESTART) == DEVICE_RESTART)               __rtn += "DEVICE_RESTART,";
    if ((enum_code & NO_FUNC_CODE_SUPPORT) == NO_FUNC_CODE_SUPPORT)   __rtn += "NO_FUNC_CODE_SUPPORT,";
    if ((enum_code & OBJECT_UNKNOWN) == OBJECT_UNKNOWN)               __rtn += "OBJECT_UNKNOWN,";
    if ((enum_code & PARAMETER_ERROR) == PARAMETER_ERROR)             __rtn += "PARAMETER_ERROR,";
    if ((enum_code & EVENT_BUFFER_OVERFLOW) == EVENT_BUFFER_OVERFLOW) __rtn += "EVENT_BUFFER_OVERFLOW,";
    if ((enum_code & ALREADY_EXECUTING) == ALREADY_EXECUTING)         __rtn += "ALREADY_EXECUTING,";
    if ((enum_code & CONFIG_CORRUPT) == CONFIG_CORRUPT)               __rtn += "CONFIG_CORRUPT,";
    if ((enum_code & RESERVED_2) == RESERVED_2)                       __rtn += "RESERVED_2,";
    if ((enum_code & RESERVED_1) == RESERVED_1)                       __rtn += "RESERVED_1,";

    return __rtn;
}

InternalIndication::InternalIndication() : flag_IIN(0x0000) {}

InternalIndication& InternalIndication::operator=(const InternalIndication& obj)
{
    if (this != &obj)
    {
        flag_IIN = obj.flag_IIN;
    }
    return *this;
}


InternalIndication::~InternalIndication() {}

}

