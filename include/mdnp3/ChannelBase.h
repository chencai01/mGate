#ifndef CHANNELBASE_H
#define CHANNELBASE_H

#include "mdnp3/Device.h"

namespace Dnp3Master
{

typedef std::map<std::string, Dnp3Point> Dnp3PointMap;

namespace FrameProcessStatus
{
enum FrameProcessStatus {idle, framming};
}

namespace ChannelType
{
enum ChannelType {UNDEFINED, SERIAL, TCP};
}

class ChannelBase
{
public:
    ChannelBase();
    virtual ~ChannelBase();

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual void send_data(std::string data) = 0;

    void set_debug(bool val=false)
    {
        _debug = val;
    }
    bool get_debug()
    {
        return _debug;
    }

    void set_name(std::string name)
    {
        _channel_name = name;
    }

    std::string get_name()
    {
        return _channel_name;
    }

protected:
    std::string _channel_name;

    std::queue<std::string> _send_data_queue;

    Dnp3PointMap _point_list;

    // = true, channel se kiem tra _send_data_info ghi xuong thiet bi
    bool _send_data_required;

    // buffer dung de luu du lieu nhan duoc
    // du lieu sau khi nhan du se duoc copy sang _full_frame de xu ly
    byte _data_buffer[MAXFRAME];

    // full frame
    byte _full_frame[MAXFRAME];

    // Nhan duoc du lieu thi se luu vao day
    // Channel se tach cac frame tu list nay de xu ly
    ByteList _byte_list;

    // Trang thai xu ly frame
    FrameProcessStatus::FrameProcessStatus _rcv_frame_stt;

    // su dung gia tri nay khi can xu ly _full_frame
    unsigned int _bytes_total;

    bool _debug;
    std::stringstream _debug_stream;

private:
};

} // Dnp3Master namespace

#endif // CHANNELBASE_H
