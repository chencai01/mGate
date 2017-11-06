#ifndef _client_h_
#define _client_h_

#include <iostream>
#include <cstring>

#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <vector>
#include <map>

#include <iomanip>
#include <signal.h>
#include <errno.h>

#include "s104/s104.h"
#include "s104/convert.h"
#include "s104/timer.h"

#define MAX_NAME_LENGHT 20

namespace S104{

using namespace std;

class session {
  public:
    string IP;
    unsigned short Port;
    bool isRunning;
    int flag;
    unsigned short asduAddress;
    short K, W;
    int T0;
    Timer T1, T2, T3;
    Timer tPeriodic, tBackground, tSBO;
    vector<S104::infItem> Data;
    vector<bool> *debug;
    vector<S104::infItem>* data;
    vector<S104::ctlItem>* control;
    map<int, int> ctl;
    vector<S104::infItem> class1, class2;
    vector<S104::SPI> *spis;
    vector<S104::DPI> *dpis;
    vector<S104::VTI> *vtis;
    vector<S104::NVA> *nvas;
    vector<S104::SVA> *svas;
    vector<S104::FP> *fps;
    SEQUENCE spiSQ, dpiSQ,vtiSQ, nvaSQ, svaSQ, fpSQ;

    //Socket stuff
    int sock;

    //function pointer defined
    typedef void (*update_dnp)(string, string);
    typedef void (*debug_print)(string);
    update_dnp update_to_dnp;
    debug_print debug_print_func;

  private:
    pthread_t _recv_thread;
    pthread_t _send_thread;
    pthread_mutex_t _mtx;
    bool enableDT;
    short Count;
    short _ns, ns, nr, NS, NR;
    vector<char> msgs;
    char buffer[261];
    unsigned char recvMsg[261], sendMsg[261];
    unsigned int recvLength, sendLength;
    int isControlling, reqUConfirm, reqINTERConfirm, reqSYNCConfirm, reqCTRLConfirm, respPERIODIC, respBACKGROUND, respINTERROGATE;
    unsigned int ptr;

    //variables for control
    int _objectAddress, __objectAddress;
    int _cot;
    SE _se;
    unsigned char _QOC;
    char _Value[4];

  public:
    session();
    ~session();
    void start();
    void add_pfunc(update_dnp pfunc);
    void add_pfunc(debug_print pfunc);
    void update(S104::infItem item);

  private:
    static void* recv_thread_functor(void* obj); // func call recv_threading
    static void* send_thread_functor(void* obj); // func call send_threading
    void recv_threading();
    void send_threading();
    void layer1debug(bool _recv, unsigned char _msg[], int _length);
    void init();
    void data2Class1();
    void addItem();
    void spi2Class2();
    void dpi2Class2();
    void vti2Class2();
    void nva2Class2();
    void sva2Class2();
    void fp2Class2();
    void EvaluateUSFormat();
    void EvaluateIFormat();
    void UConfirm();
    void INTERConfirm();
    void SYNCConfirm();
    void CTRLConfirm();
    void InterrogatedRespond();
    void PeriodicRespond();
    void BackgroundScanRespond();
    void confirmRequested();
    void class1Respond();
    void class2Respond();
    void recvSTARTDT_act();
    void recvSTARTDT_con();
    void recvSTOPDT_act();
    void recvSTOPDT_con();
    void recvTESTFR_act();
    void recvTESTFR_con();
    void recvSUPERVISORY();
    void recvInterrogation();
    void recvClockSynchronization();
    void recvSingleCommand();
    void recvDoubleCommand();
    void recvRegulatingStepCommand();
    void recvSetPointCommandNormalizedValue();
    void recvSetPointCommandScaledValue();
    void recvSetPointCommandShortFloating();
    void sendSTARTDT_act();
    void sendSTARTDT_con();
    void sendTESTFR_act();
    void sendTESTFR_con();
    void sendSTOPDT_act();
    void sendSTOPDT_con();
    void sendSupervisory();
    void sendInterrogation(COT cot);
    void sendClockSynchronization(PN pn, COT cot);
    void sendSinglePointInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot);
    void sendSinglePointInformationWithDateTime(vector<S104::infItem> items, COT cot);
    void sendDoublePointInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot);
    void sendDoublePointInformationWithDateTime(vector<S104::infItem> items, COT cot);
    void sendStepPositionInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot);
    void sendStepPositionInformationWithDateTime(vector<S104::infItem> items, COT cot);
    void sendMeasuredNormalizedValue(SEQUENCE sq, vector<S104::infItem> items, COT cot);
    void sendMeasuredScaledValue(SEQUENCE sq, vector<S104::infItem> items, COT cot);
    void sendShortFloatingPoint(SEQUENCE SQ, vector<S104::infItem> items, COT cot);
    void sendSingleCommand(PN pn, COT cot);
    void sendDoubleCommand(PN pn, COT cot);
    void sendRegulatingStepCommand(PN pn, COT cot);
    void sendSetPointCommandNormalizedValue(PN pn, COT cot);
    void sendSetPointCommandScaledValue(PN pn, COT cot);
    void sendSetPointCommandShortFloating(PN pn, COT cot);
};

}

#endif
