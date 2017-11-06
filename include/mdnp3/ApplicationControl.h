#ifndef APPLICATIONCONTROL_H_INCLUDED
#define APPLICATIONCONTROL_H_INCLUDED

namespace Dnp3Master
{

class ApplicationControl
{
public:
    ApplicationControl();
    ApplicationControl(byte byt);
    ApplicationControl(const ApplicationControl& obj);
    const byte& operator=(const byte& byt);
    ApplicationControl& operator=(const ApplicationControl& obj);

    virtual ~ApplicationControl();

    static byte create_ap_control(bool fir, bool fin, bool con, bool uns, int seq);

    void set_fir(bool value)
    {
        if (value) _ctrl_byte |= 0x80;
        else _ctrl_byte &= 0x7f;
    }

    bool get_fir()
    {
        return (_ctrl_byte & 0x80) == 0x80;
    }

    void set_fin(bool value)
    {
        if (value) _ctrl_byte |= 0x40;
        else _ctrl_byte &= 0xbf;
    }

    bool get_fin()
    {
        return (_ctrl_byte & 0x40) == 0x40;
    }

    void set_con(bool value)
    {
        if (value) _ctrl_byte |= 0x20;
        else _ctrl_byte &= 0xdf;
    }

    bool get_con()
    {
        return (_ctrl_byte & 0x20) == 0x20;
    }

    void set_uns(bool value)
    {
        if (value) _ctrl_byte |= 0x10;
        else _ctrl_byte &= 0xef;
    }

    bool get_uns()
    {
        return (_ctrl_byte & 0x10) == 0x10;
    }

    void set_seq(byte byt)
    {
        _ctrl_byte = (_ctrl_byte & 0xf0) | (byt & 0x0f);
    }

    byte get_seq()
    {
        return _ctrl_byte & 0x0f;
    }

    byte get_next_seq()
    {
        return (get_seq() + 1) % 16;
    }

    byte get_byte()
    {
        return _ctrl_byte;
    }

private:
    byte _ctrl_byte;
};

} // Dnp3Master namespace


#endif // APPLICATIONCONTROL_H_INCLUDED

