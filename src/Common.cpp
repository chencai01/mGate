#include "mdnp3/pch.h"
#include "Common.h"

#include "s104/util.h"

extern SerialChannel* dnp3;
extern server* iec104;
extern DebugConnector* dbg;
extern bool exit_now;
extern int sig_num;


int init_dnp3(char* path)
{
    string p = string(path);
    dnp3 = new SerialChannel();
    if(!Dnp3ConfigReader::read_config(dnp3, dnp3->get_device(), p))
    {
        delete dnp3;
        dnp3 = NULL;
        return 0;
    }
    dnp3->add_pfunc_callback(&update_to_iec);
    dnp3->add_debug_callback(&dnp_debug_data);
    return 1;
}

int init_iec(char* path)
{
    string p = string(path);
    iec104 = initServer(p);
    if (iec104 == NULL)
    {
        return 0;
    }
    else if (iec104->init() != 0)
    {
        delete iec104;
        iec104 = NULL;
        return 0;
    }
    iec104->add_update_callback(&update_to_dnp);
    iec104->add_debug_callback(&iec_debug_data);
    return 1;
}

int init_dbg()
{
    dbg = new DebugConnector();
    dbg->add_cmd_rcv_callback(&debug_cmd_rcv);
    return 1;
}

void destroy_all()
{
    if(dnp3) delete dnp3;
    if (iec104) delete iec104;
    if (dbg) delete dbg;
}

void start_all()
{
    dnp3->start();
    iec104->start();
    dbg->start();
}

void stop_all()
{
    dnp3->stop();
    iec104->stop();
    dbg->stop();
}

void signal_handler(int signum)
{
    sig_num = signum;
    exit_now = true;
}

void update_to_iec(string tag, string value, char quality)
{
    iec104->update(tag, value, quality);
}

void update_to_dnp(string tag, string value)
{
    cout << "iec_to_dnp:" << tag << "=" << value << endl;
    dnp3->send_data(tag + "=" + value);
}

void debug_cmd_rcv(string& data)
{
    cout << "debug_cmd:" << data << endl;
    vector<string> _token;
    S104::splitString(_token, data, ':');
    if (_token.size() < 2) return;
    if (_token[0] == "dnp3")
    {
        dnp3->send_data(_token[1]);
    }
    else if (_token[0] == "iec104")
    {
        iec104->setDebug(_token[1]);
    }
    else if (_token[0] == "sys")
    {
        if (_token[1] == "start")
        {
            dnp3->start();
            iec104->start();
        }
        else if (_token[1] == "stop")
        {
            dnp3->stop();
            iec104->stop();
        }
        else if (_token[1] == "exit")
        {
            sig_num = 0;
            exit_now = true;
        }
    }
}

void dnp_debug_data(string data)
{
    if (dbg->is_connected())
    {
        dbg->send_debug_data(data, 0x01);
    }
    else
    {
        dnp3->send_data("d0");
    }
}

void iec_debug_data(string data)
{
    if (dbg->is_connected())
    {
        dbg->send_debug_data(data, 0x02);
    }
    else
    {
        iec104->setDebug("0");
    }
}
