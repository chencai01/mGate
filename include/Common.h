#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <iostream>
#include <string>
#include "mdnp3/Dnp3ConfigReader.h"
#include "mdnp3/SerialChannel.h"
#include "s104/server.h"
#include "DebugConnector.h"

using namespace std;
using namespace Dnp3Master;
using namespace S104;

int init_dnp3(char* path);
int init_iec(char* path);
int init_dbg();
void destroy_all();
void start_all();
void stop_all();
void signal_handler(int signum);
void update_to_iec(string tag, string value, char quality);
void update_to_dnp(string tag, string value);
void debug_cmd_rcv(string& data);
void dnp_debug_data(string data);
void iec_debug_data(string data);


#endif // COMMON_INCLUDED
