#ifndef TRANSPORTHEADER_H_INCLUDED
#define TRANSPORTHEADER_H_INCLUDED

namespace Dnp3Master
{

class TransportHeader
{
public:
    TransportHeader();
    TransportHeader(byte byt);
    TransportHeader(const TransportHeader& obj);
    const byte& operator=(const byte& byt);
    TransportHeader& operator=(const TransportHeader& obj);

    virtual ~TransportHeader();

    void set_header(byte byt)
    {
        _header_byte = byt;
    }

    byte get_header()
    {
        return _header_byte;
    }

    void set_fin(bool value)
    {
        if (value) _header_byte |= 0x80;
        else _header_byte &= 0x7f;
    }

    bool get_fin()
    {
        return (_header_byte & 0x80) == 0x80;
    }

    void set_fir(bool value)
    {
        if (value) _header_byte |= 0x40;
        else _header_byte &= 0xbf;
    }

    bool get_fir()
    {
        return (_header_byte & 0x40) == 0x40;
    }

    void set_seq(byte seq)
    {
        _header_byte = (_header_byte & 0xc0) | (seq & 0x3f);
    }

    byte get_seq()
    {
        return _header_byte & 0x3f;
    }

    int get_next_seq()
    {
        return ((int)get_seq() + 1) % 64;
    }

private:
    byte _header_byte;

};

}


#endif // TRANSPORTHEADER_H_INCLUDED

