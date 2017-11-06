#include "mdnp3/pch.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include "Common.h"
#include "mdnp3/SerialChannel.h"
#include "s104/server.h"
#include "DebugConnector.h"

using namespace std;
using namespace Dnp3Master;
using namespace S104;


SerialChannel* dnp3 = NULL;
server* iec104 = NULL;
DebugConnector* dbg = NULL;
bool exit_now = false;
int sig_num = 0;

int main(int argc, char* argv[])
{
    signal(SIGINT, &signal_handler);
    signal(SIGTERM, &signal_handler);
    if (argc < 3)
    {
        cout << "No file input!" << endl;
        cout << "Usage: "
             << argv[0]
             << " dnp3cfg_path iec104cfg_path"
             << endl;
        return 0;
    }
    cout << "mDNP3 is initializing..." << endl;
    if (!init_dnp3(argv[1])) return 0;

    cout << "sIEC104 is initializing..." << endl;
    if (!init_iec(argv[2])) return 0;

    cout << "Debugger is initializing..." << endl;
    init_dbg();

    /* Run Apps */
    start_all();

    int c = 0;
    while (!exit_now)
    {
        if (c >= 10)
        {
            char a[5];
            sprintf(a, "%d|%d\n",
                    dnp3->dev_stt(),
                    (int)(iec104->getConnectedClient()));
            string s = string(a);
            dbg->send_debug_data(s, 0x07);
            c = 0;
        }
        c++;
        sleep(1);
    }

    dnp3->send_data("d0");
    iec104->setDebug("0");
    stop_all();
    destroy_all();
    cout << "SIGNAL: " << sig_num << "\n";
    cout << "Bye!\n";
    return 0;
}

