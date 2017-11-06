#ifndef DATALINKCONTROLFIELD_H_INCLUDED
#define DATALINKCONTROLFIELD_H_INCLUDED

namespace Dnp3Master
{

namespace FunctionCode
{
enum FunctionCode
{
    RESET_LINK        = 0x0100,  RESET_USER_PROCESS  = 0x0101,  TEST_LINK           = 0x0102,
    USERDATA_CONFIRM  = 0x0103,  USERDATA_NO_CONFIRM = 0x0104,  REQUEST_LINK_STATUS = 0x0109,
    ACK               = 0x0000,  NACK                = 0x0001,  LINK_STATUS         = 0x0011,
    LINK_NOT_FUNCTION = 0x0014,  LINK_NOT_USE        = 0x0015,  UNDEFINED           = 0xffff
};

std::string to_string(int enum_code);
}

class DataLinkControlField
{
public:
    DataLinkControlField();

    DataLinkControlField(byte byte_val);

    //Copy constructor
    DataLinkControlField(const DataLinkControlField& obj);

    //Assignment operator
    const byte& operator=(const byte& byte_value);
    DataLinkControlField& operator=(const DataLinkControlField& obj);

    virtual ~DataLinkControlField();

    bool get_dir()
    {
        return _dir;
    }

    bool get_prm()
    {
        return _prm;
    }

    bool get_fcb()
    {
        return _fcb;
    }

    bool get_fcv_dfc()
    {
        return _fcv_dfc;
    }

    FunctionCode::FunctionCode get_fc()
    {
        return _fc;
    }

    byte get_byte_value()
    {
        return _byte_val;
    }

private:
    byte _byte_val;
    bool _dir;
    bool _prm;
    bool _fcb;
    bool _fcv_dfc;
    FunctionCode::FunctionCode _fc;

    void set_bits();
};

} // Dnp3Master namespace


#endif // DATALINKCONTROLFIELD_H_INCLUDED
