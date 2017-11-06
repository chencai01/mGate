#include "mdnp3/pch.h"
#include "mdnp3/ApplicationRequestHeader.h"
#include "mdnp3/Utility.h"

namespace Dnp3Master
{

//    CONFRIM             = 0x00,  READ            = 0x01,  WRITE                = 0x02,
//    SELECT              = 0x03,  OPERATE         = 0x04,  DIRECT_OPERATE       = 0x05,
//    DIRECT_OPERATE_NR   = 0x06,  IMMED_FREEZE    = 0x07,  IMMED_FREEZE_NR      = 0x08,
//    FREEZE_CLEAR        = 0x09,  FREEZE_CLEAR_NR = 0x0a,  FREEZE_AT_TIME       = 0x0b,
//    FREEZE_AT_TIME_NR   = 0x0c,  COLD_RESTART    = 0x0d,  WARM_RESTART         = 0x0e,
//    INITIALIZE_DATA     = 0x0f,  INITIALIZE_APPL = 0x10,  ENABLE_UNSOLICITED   = 0x14,
//    DISABLE_UNSOLICITED = 0x15,  ASSIGN_CLASS    = 0x16,  DELAY_MEASURE        = 0x17,
//    RECORD_CURRENT_TIME = 0x18,  RESPONSE        = 0x81,  UNSOLICITED_RESPONSE = 0x82,
//    AUTHENTICATE_RESP   = 0x83
std::string APFunctionCode::to_string(int enum_code)
{
    using namespace APFunctionCode;
    std::string __rtn;
    switch (enum_code)
    {
    case CONFRIM:
        __rtn = "CONFIRM";
        break;
    case READ:
        __rtn = "READ";
        break;
    case WRITE:
        __rtn = "WRITE";
        break;
    case SELECT:
        __rtn = "SELECT";
        break;
    case OPERATE:
        __rtn = "OPERATE";
        break;
    case DIRECT_OPERATE:
        __rtn = "DIRECT_OPERATE";
        break;
    case DIRECT_OPERATE_NR:
        __rtn = "DIRECT_OPERATE_NR";
        break;
    case IMMED_FREEZE:
        __rtn = "IMMED_FREEZE";
        break;
    case IMMED_FREEZE_NR:
        __rtn = "IMMED_FREEZE_NR";
        break;
    case FREEZE_CLEAR:
        __rtn = "FREEZE_CLEAR";
        break;
    case FREEZE_CLEAR_NR:
        __rtn = "FREEZE_CLEAR_NR";
        break;
    case FREEZE_AT_TIME:
        __rtn = "FREEZE_AT_TIME";
        break;
    case FREEZE_AT_TIME_NR:
        __rtn = "FREEZE_AT_TIME_NR";
        break;
    case COLD_RESTART:
        __rtn = "COLD_RESTART";
        break;
    case WARM_RESTART:
        __rtn = "WARM_RESTART";
        break;
    case INITIALIZE_DATA:
        __rtn = "INITIALIZE_DATA";
        break;
    case INITIALIZE_APPL:
        __rtn = "INITIALIZE_APPL";
        break;
    case ENABLE_UNSOLICITED:
        __rtn = "ENABLE_UNSOLICITED";
        break;
    case DISABLE_UNSOLICITED:
        __rtn = "DISABLE_UNSOLICITED";
        break;
    case ASSIGN_CLASS:
        __rtn = "ASSIGN_CLASS";
        break;
    case DELAY_MEASURE:
        __rtn = "DELAY_MEASURE";
        break;
    case RECORD_CURRENT_TIME:
        __rtn = "RECORD_CURRENT_TIME";
        break;
    case RESPONSE:
        __rtn = "RESPONSE";
        break;
    case UNSOLICITED_RESPONSE:
        __rtn = "UNSOLICITED_RESPONSE";
        break;
    case AUTHENTICATE_RESP:
        __rtn = "AUTHENTICATE_RESP";
        break;
    default:
        __rtn = num_to_str(enum_code);
        break;
    }

    return __rtn;
}

} // Dnp3Master namespace
