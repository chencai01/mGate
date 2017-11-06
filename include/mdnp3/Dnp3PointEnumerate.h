#ifndef DNP3POINTENUMERATE_INCLUDED
#define DNP3POINTENUMERATE_INCLUDED

#include "mdnp3/Utility.h"

namespace Dnp3Master
{

namespace PointTypeCode
{
enum PointTypeCode
{
    undefined, si, di, bo, ai16, ai32, ai32float, ai64float,
    counter32, counter16, fcounter32, fcounter16,
    ao16, ao32, bocmd, ao16cmd, ao32cmd, bocmdstt, ao16cmdstt, ao32cmdstt,
    timeabs, timeabs_recorded, timeabs_interval, timeabs_idx_interval, timecto, timedelay,
};

std::string to_string(int enum_code);
std::string flags_to_string(int type_id, int enum_code);

} // PointTypeCode namespace

namespace DataObjectCode
{
// Object Type: Group|Variant
// You can get Group byte, Variant byte from this enum
enum DataObjectCode //: unsigned short
{
    UNDEFINE = 0x0000,

    BI_PackedFormat     = 0x0101,   // Binary Input Packed Format
    BI_Flags            = 0x0102,   // Binary Input With Flags

    BI_Event            = 0x0201,   // Binary Input Event Without Time
    BI_Event_AbsTime    = 0x0202,   // Binary Input Event With ABS Time
    BI_Event_RltTime    = 0x0203,   // Binary Input Event With RLT Time

    BO_AnyVariant   = 0x0a00,       // Binary Output Any Variant
    BO_Flags        = 0x0a02,       // Binary Ouput Status With Flags

    CROB                = 0x0c01,   // Binary Output Command - Control Relay Output Block

    BO_CMD_Event        = 0x0d01,   // Binary Output Command Event Without Time
    BO_CMD_Event_Time   = 0x0d02,   // Binary Output Command Event With Time

    Counter32_Flags = 0x1401,       // Counter 32bits With Flags
    Counter16_Flags = 0x1402,       // Counter 16bits With Flags
    Counter32       = 0x1405,       // Counter 32bits Without Flags
    Counter16       = 0x1406,       // Counter 16bits Without Flags

    FCounter32_Flags    = 0x1501,   // Frozen Counter 32bits With Flags
    FCounter16_Flags    = 0x1502,   // Frozen Counter 16bits With Flags
    FCounter32          = 0x1509,   // Frozen Counter 32bits Without Flags
    FCounter16          = 0x150a,   // Frozen Counter 16bits Without Flags

    Counter32_Event_Flags   = 0x1601,   // Counter 32bits Event With Flags
    Counter16_Event_Flags   = 0x1602,   // Counter 16bits Event With Flags
    FCounter32_Event_Flags  = 0x1701,   // Frozen Counter 32bits Event With Flags
    FCounter16_Event_Flags  = 0x1702,   // Frozen Counter 16bits Event With Flags

    AI32_Flags      = 0x1e01,       // Analog Input 32bits With Flags
    AI16_Flags      = 0x1e02,       // Analog Input 16bits With Flags
    AI32            = 0x1e03,       // Analog Input 32bits Without Flags
    AI16            = 0x1e04,       // Analog Input 16bits Without Flags
    AI32FLT_Flags   = 0x1e05,       // Analog Input 32bits Float With Flags
    AI64FLT_Flags   = 0x1e06,       // Analog Input 64bits Float With Flags

    AI32_Event          = 0x2001,   // Analog Input 32bits Event Without Time
    AI16_Event          = 0x2002,   // Analog Input 16bits Event Without Time
    AI32_Event_Time     = 0x2003,   // Analog Input 32bits Event With Time
    AI16_Event_Time     = 0x2004,   // Analog Input 16bits Event With Time
    AI32FLT_Event       = 0x2005,   // Analog Input 32bits Float Event Without Time
    AI64FLT_Event       = 0x2006,   // Analog Input 64bits Float Event Without Time
    AI32FLT_Event_Time  = 0x2007,   // Analog Input 32bits Float Event With Time
    AI64FLT_Event_Time  = 0x2008,   // Analog Input 64bits Float Event With Time

    AO_AnyVariant   = 0x2800,       // Analog Output Any Variant
    AO32_Flags      = 0x2801,       // Analog Ouput 32bits With Flags
    AO16_Flags      = 0x2802,       // Analog Ouput 16bits With Flags

    AO32 = 0x2901,                  // Analog Ouput 32bits Without Flags
    AO16 = 0x2902,                  // Analog Output 16bits Without Flags

    AO32_CMD_Event      = 0x2b01,   // Analog Output Command Event Without Time
    AO16_CMD_Event      = 0x2b02,   // Analog Output Command Event Without Time
    AO32_CMD_Event_Time = 0x2b03,   // Analog Output Command Event With Time
    AO16_CMD_Event_Time = 0x2b04,   // Analog Output Command Event With Time

    TimeDate_AbsTime                = 0x3201,   // Time And Date - ABS Time
    TimeDate_AbsTime_Interval       = 0x3202,   // Time And Date - ABS Time and Interval
    TimeDate_AbsTime_Recorded       = 0x3203,   // Time And Date - ABS Time at Last Recored Time
    TimeDate_AbsTime_IdxInterval    = 0x3204,   // Time And Date - Indexed ABS Time and Long Interval

    TimeDate_CTO_AbsTime_Sync   = 0x3301,   // Time And Date - Common Time of Occur, synchronized
    TimeDate_CTO_AbsTime_Unsync = 0x3302,   // Time And Date - Common Time of Occur, unsynchronized

    TimeDelay_Coarse    = 0x3401,   // Time Delay - Coarse (resolution of 1 sec)
    TimeDelay_Fine      = 0x3402,   // Time Delay - Fine (resolution of 1 msec)

    ClassObj_0 = 0x3c01,            // Class 0 Data
    ClassObj_1 = 0x3c02,            // Class 1 Data
    ClassObj_2 = 0x3c03,            // Class 2 Data
    ClassObj_3 = 0x3c04,            // Class 3 Data

    IIN_PackedFormat = 0x5001       // Internal Indications Packed Format
};
}

namespace SingleInputFlagCode
{
enum SingleInputFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST      = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, CHATTER_FILTER = 0x20,
    RESERVED      = 0x40, STATE        = 0x80
};

std::string to_string(int enum_code);
}

namespace DoubleInputFlagCode
{
enum DoubleInputFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST      = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, CHATTER_FILTER = 0x20,
    STATE1        = 0x40, STATE2       = 0x80
};

std::string to_string(int enum_code);
}

namespace BinaryOutputFlagCode
{
enum BinaryOutputFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, RESERVED1 = 0x20,
    RESERVED2     = 0x40, STATE        = 0x80
};

std::string to_string(int enum_code);
}

namespace AnalogInputFlagCode
{
enum AnalogInputFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST  = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, OVER_RANGE = 0x20,
    REFERENCE_ERR = 0x40, RESERVED     = 0x80
};

std::string to_string(int enum_code);
}

namespace AnalogOutputFlagCode
{
enum AnalogOutputFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST  = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, OVER_RANGE = 0x20,
    REFERENCE_ERR = 0x40, RESERVED     = 0x80
};

std::string to_string(int enum_code);
}

namespace CounterFlagCode
{
enum CounterFlagCode //: byte
{
    ONLINE        = 0x01, RESTART      = 0x02, COMM_LOST = 0x04,
    REMOTE_FORCED = 0x08, LOCAL_FORCED = 0x10, ROLLOVER  = 0x20,
    DISCONTINUITY = 0x40, RESERVED     = 0x80
};

std::string to_string(int enum_code);
}

namespace CmdOperTypeCode
{
enum CmdOperTypeCode //: byte
{
    nul = 0, pulseon = 1, pulseoff = 2, latchon = 3, latchoff = 4
};

std::string to_string(int enum_code);
}

namespace CmdTrClsCode
{
enum CmdTrClsCode //: byte
{
    nul = 0, close = 1, trip = 2, reserved = 3
};

std::string to_string(int enum_code);
}

namespace CmdTypeCode
{
enum CmdTypeCode //: byte
{
    sop, dop, dona
};

std::string to_string(int enum_code);
}

namespace CmdStatusCode
{
enum CmdStatusCode //: byte
{
    SUCCESS        = 0,  TIMEOUT         = 1,   NO_SELECT      = 2,
    FORMAT_ERROR   = 3,  NOT_SUPPORT     = 4,   ALREADY_ACTIVE = 5,
    HARDWARE_ERROR = 6,  LOCAL           = 7,   TOO_MANY       = 8,
    NOT_AUTHORIZED = 9,  AUTO_INHIBIT    = 10,  PROCESS_LIMIT  = 11,
    OUT_OF_RANGE   = 12, NON_PATICIPATE  = 126, UNDEFINED      = 127
};

std::string to_string(int enum_code);
}


} // Dnp3Master namespace


#endif // DNP3POINTENUMERATE_INCLUDED
