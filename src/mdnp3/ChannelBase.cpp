#include "mdnp3/pch.h"
#include "mdnp3/ChannelBase.h"

namespace Dnp3Master
{
ChannelBase::ChannelBase()
{
    //ctor
    _send_data_required = false;
    _bytes_total = false;
    _debug = false;
    _rcv_frame_stt = FrameProcessStatus::idle;
}

ChannelBase::~ChannelBase()
{
    //dtor
}

} // Dnp3Master namespace
