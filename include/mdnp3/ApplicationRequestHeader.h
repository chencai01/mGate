#ifndef APPLICATIONREQUESTHEADER_H_INCLUDED
#define APPLICATIONREQUESTHEADER_H_INCLUDED


#include "mdnp3/ApplicationControl.h"

namespace Dnp3Master
{

namespace APFunctionCode
{
enum APFunctionCode //: byte
{
    CONFRIM             = 0x00,  READ            = 0x01,  WRITE                = 0x02,
    SELECT              = 0x03,  OPERATE         = 0x04,  DIRECT_OPERATE       = 0x05,
    DIRECT_OPERATE_NR   = 0x06,  IMMED_FREEZE    = 0x07,  IMMED_FREEZE_NR      = 0x08,
    FREEZE_CLEAR        = 0x09,  FREEZE_CLEAR_NR = 0x0a,  FREEZE_AT_TIME       = 0x0b,
    FREEZE_AT_TIME_NR   = 0x0c,  COLD_RESTART    = 0x0d,  WARM_RESTART         = 0x0e,
    INITIALIZE_DATA     = 0x0f,  INITIALIZE_APPL = 0x10,  ENABLE_UNSOLICITED   = 0x14,
    DISABLE_UNSOLICITED = 0x15,  ASSIGN_CLASS    = 0x16,  DELAY_MEASURE        = 0x17,
    RECORD_CURRENT_TIME = 0x18,  RESPONSE        = 0x81,  UNSOLICITED_RESPONSE = 0x82,
    AUTHENTICATE_RESP   = 0x83
};

std::string to_string(int enum_code);
}


class ApplicationRequestHeader
{
public:
    ApplicationRequestHeader() : AC(0x00), FC(APFunctionCode::CONFRIM) {}

    virtual ~ApplicationRequestHeader() {}

    ApplicationControl AC;
    APFunctionCode::APFunctionCode FC;
};

}

#endif // APPLICATIONREQUESTHEADER_H_INCLUDED
