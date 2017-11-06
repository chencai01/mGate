#include "mdnp3/pch.h"
#include "mdnp3/Dnp3ConfigReader.h"

namespace Dnp3Master
{

Dnp3ConfigReader::Dnp3ConfigReader()
{
    //ctor
}

Dnp3ConfigReader::~Dnp3ConfigReader()
{
    //dtor
}

int Dnp3ConfigReader::init_serialchannel(SerialChannel* channel, std::ifstream& cfg_stream)
{
// [channel]
// name=channel1
// port=/dev/ttyS1
// baudrate=9600
// char_size=8
// stop_bits=1
// parity=none
// flow_ctrl=none

    using namespace std;
    string __line;
    char __c;
    vector<string> __token;
    while (getline(cfg_stream, __line))
    {
        #ifdef DNP3_DEBUG
        std::cout << __line << std::endl;
        #endif
        __token.clear();
        split_string(__token, __line, '=');
        if (__token.size() > 1)
        {
            if (__token[0] == "name") // channel name
                channel->set_name(__token[1]);
            else if (__token[0] == "port") // port
                channel->set_port_name(__token[1]);
            else if (__token[0] == "baudrate") // baudrate
                channel->set_baud_rate(str_to_num<unsigned int>(__token[1]));
            else if (__token[0] == "char_size") // character size
            {
                if (__token[1] == "5")
                    channel->set_character_size(CharacterSizeType::five);
                else if (__token[1] == "6")
                    channel->set_character_size(CharacterSizeType::six);
                else if (__token[1] == "7")
                    channel->set_character_size(CharacterSizeType::seven);
                else
                    channel->set_character_size(CharacterSizeType::eight);
            }
            else if (__token[0] == "stop_bits") // stop bits
            {
                if (__token[1] == "2")
                    channel->set_stop_bits(StopBitsType::two);
                else if (__token[1] == "1.5")
                    channel->set_stop_bits(StopBitsType::onepointfive);
                else
                    channel->set_stop_bits(StopBitsType::one);
            }
            else if (__token[0] == "parity") // parity
            {
                if (__token[1] == "even")
                    channel->set_parity(ParityType::even);
                else if (__token[1] == "odd")
                    channel->set_parity(ParityType::odd);
                else
                    channel->set_parity(ParityType::none);
            }
            else if (__token[0] == "flow_ctrl") // flow control
            {
                if (__token[1] == "software")
                    channel->set_flow_control(FlowControlType::software);
                else if (__token[1] == "hardware")
                    channel->set_flow_control(FlowControlType::hardware);
                else
                    channel->set_flow_control(FlowControlType::none);
            }
        }
        __c = (char)(cfg_stream.peek());
        if (__c == '[') break; // reached [device]
    }

    return 1;
}

int Dnp3ConfigReader::init_device(Device* dev, std::ifstream& cfg_stream)
{
    //[device]
    //name=device1
    //desc=recloser
    //enable=1
    //src=0
    //dest=1
    //count_max=-1
    //time_ready=100
    //time_class0=5000
    //dl_retries=3
    //dl_timeout=1000
    //ap_timeout=5000
    //poll_class=1,0,0,0
    //uns_class=0,0,0
    //time_sync=1

    using namespace std;
    string __line;
    char __c;
    vector<string> __token;
    while (getline(cfg_stream, __line))
    {
        #ifdef DNP3_DEBUG
        std::cout << __line << std::endl;
        #endif
        __token.clear();
        split_string(__token, __line, '=');
        if (__token.size() > 1)
        {
            if (__token[0] == "name")
                dev->set_name(__token[1]);
            else if (__token[0] == "desc")
                dev->set_description(__token[1]);
            else if (__token[0] == "enable")
            {
                if (__token[1] == "1")
                    dev->set_enable(true);
                else
                    dev->set_enable(false);
            }
            else if (__token[0] == "src")
                dev->get_datalink_loader()->set_src_address(str_to_num<unsigned short>(__token[1]));
            else if (__token[0] == "dest")
                dev->get_datalink_loader()->set_dest_address(str_to_num<unsigned short>(__token[1]));
            else if (__token[0] == "count_max")
                dev->set_poll_count_max(str_to_num<int>(__token[1]));
            else if (__token[0] == "time_ready")
                dev->set_time_ready(str_to_num<long>(__token[1]));
            else if (__token[0] == "time_class0")
                dev->set_class0_time(str_to_num<long>(__token[1]));
            else if (__token[0] == "dl_retries")
                dev->get_datalink_loader()->set_retry_max(str_to_num<int>(__token[1]));
            else if (__token[0] == "dl_timeout")
                dev->get_datalink_loader()->set_timeout(str_to_num<long>(__token[1]));
            else if (__token[0] == "ap_timeout")
                dev->get_app_loader()->set_timeout(str_to_num<long>(__token[1]));
            else if (__token[0] == "time_sync")
            {
                if (__token[1] == "1") dev->set_timesync_enable(true);
                else dev->set_timesync_enable(false);
            }
            else if (__token[0] == "poll_class")
            {
                __line = __token[1]; // Luu tam thoi
                __token.clear();
                split_string(__token, __line, ',');
                if (__token.size() > 3)
                {
                    int _0 = str_to_num<int>(__token[0]); // class 0
                    int _1 = str_to_num<int>(__token[1]); // class 1
                    int _2 = str_to_num<int>(__token[2]); // class 2
                    int _3 = str_to_num<int>(__token[3]); // class 3
                    dev->set_poll(_0==1, _1==1, _2==1, _3==1);
                }
            }
            else if (__token[0] == "uns_class")
            {
                __line = __token[1]; // Luu tam thoi
                __token.clear();
                split_string(__token, __line, ',');
                if (__token.size() > 2)
                {
                    int _1 = str_to_num<int>(__token[1]); // class 1
                    int _2 = str_to_num<int>(__token[2]); // class 2
                    int _3 = str_to_num<int>(__token[3]); // class 3
                    dev->set_uns(_1==1, _2==1, _3==1);
                }
            }
        }

        __c = (char)(cfg_stream.peek());
        if (__c == '[') break; // reached [channel]
    }

    return 1;
}

bool Dnp3ConfigReader::read_config(SerialChannel* channel, Device* dev, std::string& path)
{
    std::ifstream _cfg_stream;
    _cfg_stream.open(path.c_str());
    std::string __line;
    if (!_cfg_stream.good()) return false;
    while (std::getline(_cfg_stream, __line))
    {
        #ifdef DNP3_DEBUG
        std::cout << __line << std::endl;
        #endif
        if (__line == "[channel]")
        {
            Dnp3ConfigReader::init_serialchannel(channel, _cfg_stream);
        }
        else if (__line == "[device]")
        {
            Dnp3ConfigReader::init_device(dev, _cfg_stream);
        }
    }
    _cfg_stream.close();
    return true;
}


} // Dnp3Master namespace
