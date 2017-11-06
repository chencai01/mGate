#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <map>
#include <pthread.h>

#include "s104/session.h"
#include "s104/s104.h"

#include "s104/bitconverter.h"
#include "s104/convert.h"

#define PORT 2404
#define MAX_CLIENT 4

namespace S104{

using namespace std;

class server {

  public:
    bool is_running;
    session sessions[MAX_CLIENT];
    //vector<S104::session> sessions;
    map<string, int> IPs;
    unsigned short asduAddress;
    short K, W;
    int MaxClient, T0, T1, T2, T3;
    int Periodic, Background, SBOTimeout;
    vector<bool> debug;
    vector<S104::SPI> spis;
    vector<S104::DPI> dpis;
    vector<S104::VTI> vtis;
    vector<S104::NVA> nvas;
    vector<S104::SVA> svas;
    vector<S104::FP> fps;
    vector<S104::infItem> data;
    vector<S104::ctlItem> control;
    map<string, string> inf;
    map<int, int> ctl;
    SEQUENCE spiSQ, dpiSQ, vtiSQ, nvaSQ, svaSQ, fpSQ;

    //Socket stuff
    int serverSock;
    struct sockaddr_in serverAddr, clientAddr;

    //function pointer defined
    typedef void (*update_dnp)(string, string);
    typedef void (*debug_print)(string);

  public:
    server();
    ~server();
    static void* run_thread_functor(void* obj);
    static void* rand_thread_functor(void* obj);
    void start();
    void stop();
    int init();
    void run();
    void random();
    int getConnectedClient();
    void add_update_callback(update_dnp pfunc);
    void add_debug_callback(debug_print pfunc);
    void update(string _ref, string _value, char _quality);
    void setDebug(string _debug);
    void addSPI(string _ref, int _address);
    void addDPI(string _ref, int _address);
    void addDPI(string _ref1, string _ref2, int _address);
    void addVTI(string _ref, int _address);
    void addNVA(string _ref, int _address);
    void addSVA(string _ref, int _address);
    void addFP(string _ref, int _address);
    void addFP(string _ref, int _address, float _a, float _b);
    void addSCO(string _ref, int _address, string _sbo);
    void addDCO(string _ref, int _address, string _sbo);
    void addDCO(string _ref1, string _ref2, int _address, string _sbo);
    void addRCO(string _ref, int _address, string _sbo);
    void addRCO(string _ref1, string _ref2, int _address, string _sbo);
    void addNVO(string _ref, int _address, string _sbo);
    void addSVO(string _ref, int _address, string _sbo);
    void addFPO(string _ref, int _address, string _sbo);

  private:
    update_dnp update_to_dnp;
    debug_print debug_print_funct;
    pthread_t run_thread, rand_thread;
    pthread_mutex_t mtx_lock;
    void initSession(session *s);
};

}

#endif // SERVER_H
