#ifndef SERIALCHANNELBASE_H
#define SERIALCHANNELBASE_H

#include "mdnp3/ChannelBase.h"

namespace Dnp3Master
{

namespace FlowControlType
{
enum FlowControlType {none, software, hardware};
}

namespace ParityType
{
enum ParityType {none, odd, even};
}

namespace StopBitsType
{
enum StopBitsType {one, onepointfive, two};
}

namespace CharacterSizeType
{
enum CharacterSizeType {five, six, seven, eight};
}

typedef unsigned int BaudrateType;

class SerialChannelBase : public ChannelBase
{

public:
    SerialChannelBase();
    virtual ~SerialChannelBase();

    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void send_data(std::string data) = 0;

    void set_port_name(std::string name)
    {
        _port_name = name;
    }

    std::string get_port_name()
    {
        return _port_name;
    }

    void set_baud_rate(unsigned int rate)
    {
        _baudrate = rate;
    }

    void set_flow_control(FlowControlType::FlowControlType flw_type)
    {
        _flow_control = flw_type;
    }

    void set_parity(ParityType::ParityType par_type)
    {
        _parity = par_type;
    }

    void set_stop_bits(StopBitsType::StopBitsType stop_type)
    {
        _stop_bit = stop_type;
    }

    void set_character_size(CharacterSizeType::CharacterSizeType char_type)
    {
        _char_size = char_type;
    }

protected:
    bool _is_running;   // Tien trinh polling dang chay
    bool _is_receving;  // Tien trinh read data dang chay

    /** PARAMETER OF SERIAL PORT **/
    std::string _port_name;
    BaudrateType _baudrate;
    FlowControlType::FlowControlType _flow_control;
    ParityType::ParityType _parity;
    StopBitsType::StopBitsType _stop_bit;
    CharacterSizeType::CharacterSizeType _char_size;
    /******************************/

private:
};

} // Dnp3Master namespace

#endif // SERIALCHANNELBASE_H
