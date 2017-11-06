#ifndef INTERNALINDICATION_H_INCLUDED
#define INTERNALINDICATION_H_INCLUDED

namespace Dnp3Master
{

namespace IINFlags
{
enum IINFlags //: unsigned short
{
    BROADCAST      = 0x0001,      NO_FUNC_CODE_SUPPORT  = 0x0100,
    CLASS_1_EVENTS = 0x0002,      OBJECT_UNKNOWN        = 0x0200,
    CLASS_2_EVENTS = 0x0004,      PARAMETER_ERROR       = 0x0400,
    CLASS_3_EVENTS = 0x0008,      EVENT_BUFFER_OVERFLOW = 0x0800,
    NEED_TIME      = 0x0010,      ALREADY_EXECUTING     = 0x1000,
    LOCAL_CONTROL  = 0x0020,      CONFIG_CORRUPT        = 0x2000,
    DEVICE_TROUBLE = 0x0040,      RESERVED_2            = 0x4000,
    DEVICE_RESTART = 0x0080,      RESERVED_1            = 0x8000
};

std::string to_string(int enum_code);

} // IINFlags namespace

class InternalIndication
{
public:
    InternalIndication();
    InternalIndication& operator=(const InternalIndication& obj);

    virtual ~InternalIndication();

    void set_IIN(byte first, byte second)
    {
        flag_IIN = ((unsigned short)(second) << 8) | (unsigned short)(first);
    }

    void set_IIN(IINFlags::IINFlags flag)
    {
        flag_IIN |= static_cast<unsigned short>(flag);
    }

    bool class1_events()
    {
        return (flag_IIN & 0x0002) == 0x0002;
    }

    bool class2_events()
    {
        return (flag_IIN & 0x0004) == 0x0004;
    }

    bool class3_events()
    {
        return (flag_IIN & 0x0008) == 0x0008;
    }

    bool need_time()
    {
        return (flag_IIN & 0x0010) == 0x0010;
    }

    bool device_restart()
    {
        return (flag_IIN & 0x0080) == 0x0080;
    }

    unsigned short flag_IIN;

};

}


#endif // INTERNALINDICATION_H_INCLUDED

