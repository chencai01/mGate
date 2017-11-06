#ifndef DNP3CONFIGREADER_H
#define DNP3CONFIGREADER_H

#include "mdnp3/SerialChannel.h"

namespace Dnp3Master
{
class Dnp3ConfigReader
{
public:
    Dnp3ConfigReader();
    virtual ~Dnp3ConfigReader();

    static int init_serialchannel(SerialChannel* channel, std::ifstream& cfg_stream);
    static int init_device(Device* dev, std::ifstream& cfg_stream);
    static bool read_config(SerialChannel* channel, Device* dev, std::string& path);
protected:

private:
};


} // Dnp3Master namespace
#endif // DNP3CONFIGREADER_H
