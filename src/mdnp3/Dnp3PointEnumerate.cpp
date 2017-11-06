#include "mdnp3/pch.h"
#include "mdnp3/Dnp3PointEnumerate.h"

namespace Dnp3Master
{

std::string PointTypeCode::to_string(int enum_code)
{
    using namespace PointTypeCode;
    std::string __rtn;
    switch (enum_code)
    {
    case undefined:
        __rtn = "undefined";
        break;
    case si:
        __rtn = "si";
        break;
    case di:
        __rtn = "di";
        break;
    case bo:
        __rtn = "bo";
        break;
    case ai16:
        __rtn = "ai16";
        break;
    case ai32:
        __rtn = "ai32";
        break;
    case ai32float:
        __rtn = "ai32float";
        break;
    case ai64float:
        __rtn = "ai64float";
        break;
    case counter32:
        __rtn = "counter32";
        break;
    case counter16:
        __rtn = "counter16";
        break;
    case fcounter32:
        __rtn = "fcounter32";
        break;
    case fcounter16:
        __rtn = "fcounter16";
        break;
    case ao16:
        __rtn = "ao16";
        break;
    case ao32:
        __rtn = "ao32";
        break;
    case bocmd:
        __rtn = "bocmd";
        break;
    case ao16cmd:
        __rtn = "ao16cmd";
        break;
    case ao32cmd:
        __rtn = "ao32cmd";
        break;
    default:
        __rtn = num_to_str(enum_code);
    }
    return __rtn;
}

std::string PointTypeCode::flags_to_string(int type_id, int enum_code)
{
    using namespace PointTypeCode;
    std::string __rtn;
    switch (type_id)
    {
    case si:
        __rtn = SingleInputFlagCode::to_string(enum_code);
        break;
    case di:
        __rtn = DoubleInputFlagCode::to_string(enum_code);
        break;
    case bo:
        __rtn = BinaryOutputFlagCode::to_string(enum_code);
        break;
    case ai16:
    case ai32:
    case ai32float:
    case ai64float:
        __rtn = AnalogInputFlagCode::to_string(enum_code);
        break;
    case counter32:
    case counter16:
    case fcounter32:
    case fcounter16:
        __rtn = CounterFlagCode::to_string(enum_code);
        break;
    case ao16:
    case ao32:
        __rtn = AnalogOutputFlagCode::to_string(enum_code);
        break;
    default:
        __rtn = num_to_str(enum_code);
    }
    return __rtn;
}

//    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST      = 0x04,
//    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, CHATTER_FILTER = 0x20,
//    RESERVED      = 0x40, STATE        = 0x80
std::string SingleInputFlagCode::to_string(int enum_code)
{
    using namespace SingleInputFlagCode;
    std::string __rtn = "OFFLINE";
    if ((enum_code & ONLINE) == ONLINE)                     __rtn = "ONLINE";
    if ((enum_code & RESTART) == RESTART)                   __rtn += ",RESTART";
    if ((enum_code & COMM_LOST) == COMM_LOST)               __rtn += ",COMM_LOST";
    if ((enum_code & REMOTE_FORCED) == REMOTE_FORCED)       __rtn += ",REMOTE_FORCED";
    if ((enum_code & LOCAL_FORCED) == LOCAL_FORCED)         __rtn += ",LOCAL_FORCED";
    if ((enum_code & CHATTER_FILTER) == CHATTER_FILTER)     __rtn += ",CHATTER_FILTER";
    return __rtn;
}

//    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST      = 0x04,
//    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, CHATTER_FILTER = 0x20,
//    RESERVED      = 0x40, STATE        = 0x80
std::string DoubleInputFlagCode::to_string(int enum_code)
{
    return SingleInputFlagCode::to_string(enum_code);
}

//    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST = 0x04,
//    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, RESERVED1 = 0x20,
//    RESERVED2     = 0x40, STATE        = 0x80
std::string BinaryOutputFlagCode::to_string(int enum_code)
{
    using namespace BinaryOutputFlagCode;
    std::string __rtn = "OFFLINE";
    if ((enum_code & ONLINE) == ONLINE)                 __rtn = "ONLINE";
    if ((enum_code & RESTART) == RESTART)               __rtn += ",RESTART";
    if ((enum_code & COMM_LOST) == COMM_LOST)           __rtn += ",COMM_LOST";
    if ((enum_code & REMOTE_FORCED) == REMOTE_FORCED)   __rtn += ",REMOTE_FORCED";
    if ((enum_code & LOCAL_FORCED) == LOCAL_FORCED)     __rtn += ",LOCAL_FORCED";
    return __rtn;
}

//    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST  = 0x04,
//    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, OVER_RANGE = 0x20,
//    REFERENCE_ERR = 0x40, RESERVED     = 0x80
std::string AnalogInputFlagCode::to_string(int enum_code)
{
    using namespace AnalogInputFlagCode;
    std::string __rtn = "OFFLINE";
    if ((enum_code & ONLINE) == ONLINE)                 __rtn = "ONLINE";
    if ((enum_code & RESTART) == RESTART)               __rtn += ",RESTART";
    if ((enum_code & COMM_LOST) == COMM_LOST)           __rtn += ",COMM_LOST";
    if ((enum_code & REMOTE_FORCED) == REMOTE_FORCED)   __rtn += ",REMOTE_FORCED";
    if ((enum_code & LOCAL_FORCED) == LOCAL_FORCED)     __rtn += ",LOCAL_FORCED";
    if ((enum_code & OVER_RANGE) == OVER_RANGE)         __rtn += ",OVER_RANGE";
    if ((enum_code & REFERENCE_ERR) == REFERENCE_ERR)   __rtn += ",REFERENCE_ERR";
    return __rtn;
}

std::string AnalogOutputFlagCode::to_string(int enum_code)
{
    return AnalogInputFlagCode::to_string(enum_code);
}

//    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST = 0x04,
//    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, ROLLOVER  = 0x20,
//    DISCONTINUITY = 0x40, RESERVED     = 0x80
std::string CounterFlagCode::to_string(int enum_code)
{
    using namespace CounterFlagCode;
    std::string __rtn = "OFFLINE";
    if ((enum_code & ONLINE) == ONLINE)                 __rtn = "ONLINE";
    if ((enum_code & RESTART) == RESTART)               __rtn += ",RESTART";
    if ((enum_code & COMM_LOST) == COMM_LOST)           __rtn += ",COMM_LOST";
    if ((enum_code & REMOTE_FORCED) == REMOTE_FORCED)   __rtn += ",REMOTE_FORCED";
    if ((enum_code & LOCAL_FORCED) == LOCAL_FORCED)     __rtn += ",LOCAL_FORCED";
    if ((enum_code & ROLLOVER) == ROLLOVER)             __rtn += ",ROLLOVER";
    if ((enum_code & DISCONTINUITY) == DISCONTINUITY)   __rtn += ",DISCONTINUITY";
    return __rtn;
}

//nul = 0, pulseon = 1, pulseoff = 2, latchon = 3, latchoff = 4
std::string CmdOperTypeCode::to_string(int enum_code)
{
    using namespace CmdOperTypeCode;
    std::string __rtn;
    switch (enum_code)
    {
    case nul:
        __rtn = "NUL";
        break;
    case pulseon:
        __rtn = "PULSE_ON";
        break;
    case pulseoff:
        __rtn = "PULSE_OFF";
        break;
    case latchon:
        __rtn = "LATCH_ON";
        break;
    case latchoff:
        __rtn = "LATCH_OFF";
        break;
    default:
        __rtn = num_to_str(enum_code);
        break;
    }
    return __rtn;
}

//nul = 0, close = 1, trip = 2, reserved = 3
std::string CmdTrClsCode::to_string(int enum_code)
{
    using namespace CmdTrClsCode;
    std::string __rtn;
    switch (enum_code)
    {
    case nul:
        __rtn = "NUL";
        break;
    case close:
        __rtn = "CLOSE";
        break;
    case trip:
        __rtn = "TRIP";
        break;
    case reserved:
        __rtn = "RESERVED";
        break;
    default:
        __rtn = num_to_str(enum_code);
        break;
    }
    return __rtn;
}

//sop, dop, dona
std::string CmdTypeCode::to_string(int enum_code)
{
    using namespace CmdTypeCode;
    std::string __rtn;
    switch (enum_code)
    {
    case sop:
        __rtn = "SELECT_BEFORE_OPERATE";
        break;
    case dop:
        __rtn = "DIRECT_OPERATE";
        break;
    case dona:
        __rtn = "DIRECT_OPERATE_NO_ACK";
        break;
    default:
        __rtn = num_to_str(enum_code);
        break;
    }
    return __rtn;
}

//    SUCCESS        = 0,  TIMEOUT         = 1,   NO_SELECT      = 2,
//    FORMAT_ERROR   = 3,  NOT_SUPPORT     = 4,   ALREADY_ACTIVE = 5,
//    HARDWARE_ERROR = 6,  LOCAL           = 7,   TOO_MANY       = 8,
//    NOT_AUTHORIZED = 9,  AUTO_INHIBIT    = 10,  PROCESS_LIMIT  = 11,
//    OUT_OF_RANGE   = 12, NON_PATICIPATE  = 126, UNDEFINED      = 127
std::string CmdStatusCode::to_string(int enum_code)
{
    using namespace CmdStatusCode;
    std::string __rtn;
    switch (enum_code)
    {
    case SUCCESS:
        __rtn = "SUCCESS";
        break;
    case TIMEOUT:
        __rtn = "TIMEOUT";
        break;
    case NO_SELECT:
        __rtn = "NO_SELECT";
        break;
    case FORMAT_ERROR:
        __rtn = "FORMAT_ERROR";
        break;
    case NOT_SUPPORT:
        __rtn = "NOT_SUPPORT";
        break;
    case ALREADY_ACTIVE:
        __rtn = "ALREADY_ACTIVE";
        break;
    case HARDWARE_ERROR:
        __rtn = "HARDWARE_ERROR";
        break;
    case LOCAL:
        __rtn = "LOCAL";
        break;
    case TOO_MANY:
        __rtn = "TOO_MANY";
        break;
    case NOT_AUTHORIZED:
        __rtn = "NOT_AUTHORIZED";
        break;
    case AUTO_INHIBIT:
        __rtn = "AUTO_INHIBIT";
        break;
    case PROCESS_LIMIT:
        __rtn = "PROCESS_LIMIT";
        break;
    case OUT_OF_RANGE:
        __rtn = "OUT OF RANGE";
        break;
    case NON_PATICIPATE:
        __rtn = "NON_PATICIPATE";
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


} // Dnp3Master namespace
