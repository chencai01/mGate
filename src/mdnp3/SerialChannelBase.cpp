#include "mdnp3/pch.h"
#include "mdnp3/SerialChannelBase.h"

namespace Dnp3Master
{
SerialChannelBase::SerialChannelBase() : ChannelBase()
{
    //ctor
    _bytes_total = 0;
    _rcv_frame_stt = FrameProcessStatus::idle;
    _is_running = false;
    _is_receving = false;
    _send_data_required = false;

    _port_name = "";
    _baudrate = 9600;
    _char_size = CharacterSizeType::eight;
    _stop_bit = StopBitsType::one;
    _parity = ParityType::none;
    _flow_control = FlowControlType::none;
}

SerialChannelBase::~SerialChannelBase()
{
    //dtor
}

} // Dnp3Master namespace
