#include "s104/session.h"
#include "s104/s104.h"
#include "s104/bitconverter.h"

namespace S104{

using namespace std;

session::session() {
  this->update_to_dnp = NULL;
  this->debug_print_func = NULL;
  this->isRunning = false;
  this->sock = -1;
  this->data = &this->Data;
  pthread_mutex_init(&_mtx, NULL);
}

session::~session(){
    pthread_mutex_destroy(&_mtx);
}

void session::add_pfunc(update_dnp pfunc){
    update_to_dnp = pfunc;
}

void session::add_pfunc(debug_print pfunc){
    debug_print_func = pfunc;
}

void session::update(S104::infItem item){
    if (isRunning){
        pthread_mutex_lock(&_mtx);
        data->push_back(item);
        pthread_mutex_unlock(&_mtx);
    }
}

void session::start(){
    init();
    pthread_create(&_recv_thread, NULL, &session::recv_thread_functor, (void*)this);
    pthread_create(&_send_thread, NULL, &session::send_thread_functor, (void*)this);
}

void session::init(){
    this->isRunning = true;
    this->isControlling = 0;
    this->reqUConfirm = 0;
    this->reqSYNCConfirm = 0;
    this->reqINTERConfirm = 0;
    this->reqCTRLConfirm= 0;
    this->respINTERROGATE = 0;
    this->respBACKGROUND = 0;
    this->respPERIODIC = 0;
    this->enableDT = false;
    this->Count = 0;
    this->ptr = 0;
    this->_ns = 0;
    this->ns = 0;
    this->nr = 0;
    this->NS = 0;
    this->NR = 0;
    this->T1.stop();
    this->T2.stop();
    this->T3.stop();
}

void* session::recv_thread_functor(void* obj){
    session* __obj = (session*)obj;
    __obj->recv_threading();
    return NULL;
}

void* session::send_thread_functor(void* obj){
    session* __obj = (session*)obj;
    __obj->send_threading();
    return NULL;
}

void session::recv_threading(){
    int n;
    while(isRunning){
        n = recv(sock, buffer, sizeof buffer, 0);
        if(n <= 0){
            isRunning = false;
            break;
        }
        else{
            for (int i = 0; i < n; i++){
                char ch = buffer[i];
                msgs.push_back(ch);
            }
            while(msgs.size()>=6){
                 recvLength = msgs[1] + 2;
                 if(msgs.size() >= recvLength){
                    for (unsigned int i = 0; i < recvLength; i++) recvMsg[i] = msgs[i];
                    msgs.erase(msgs.begin(), msgs.begin() + recvLength);
                    layer1debug(true, recvMsg, recvLength);
                    if(recvLength == 6) EvaluateUSFormat();
                    else EvaluateIFormat();
                }
            }
        }
    }
}

void session::send_threading(){
    T3.start();
    while(isRunning){
        if(!T1.is_timeout()){
            if(reqUConfirm > 0) UConfirm();
            else if(enableDT){
                if(tSBO.is_timeout()) isControlling = 0;
                if(reqCTRLConfirm > 0) CTRLConfirm();
                else if(reqSYNCConfirm > 0) SYNCConfirm();
                else if(reqINTERConfirm > 0) INTERConfirm();
                else if(data->size() > 0) class1Respond();
                else class2Respond();
            }
            usleep(100000);
        }
        else{
            this->isRunning = false;
            break;
        }
    }
    pthread_mutex_lock(&_mtx);
    this->data->clear();
    pthread_mutex_unlock(&_mtx);
    close(this->sock);
    this->sock = -1;
}

void session::layer1debug(bool _recv, unsigned char _msg[], int _length){
    stringstream _debug_stream;
    if(debug->at(0) == true){
        if(_length <= 6){
            if(_recv) _debug_stream << "<--- :";
            else _debug_stream << "---> :";
            _debug_stream << std::hex << std::setfill('0');
            for(int i = 0; i < _length; i++){
                _debug_stream << " " << std::setw(2) << (int)_msg[i];
            }
            _debug_stream << "\n";
        }
        else{
            if(_recv) _debug_stream << "<--- :";
            else _debug_stream << "---> :";
            _debug_stream << std::hex << std::setfill('0');
            for (int i = 0; i < 6; i++)
            {
                _debug_stream << " " << std::setw(2) << (int)_msg[i];
            }
            _debug_stream << "\n";
            int _row = _length / 16;
            for (int i = 0; i <= _row; i++)
            {
                _debug_stream << "      ";
                for (int j = 16 * i + 6; (j < _length) && (j < 16 * i + 22); j++)
                {
                     _debug_stream << " " << std::setw(2) << (int)_msg[j];
                }
                _debug_stream << "\n";
            }
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
}

void session::data2Class1(){
    bool _done = false;
    S104::infItem _item;
    class1.clear();
    while(!_done){
        if(data->size() > 1){
            if(data->at(0).typeID == data->at(1).typeID){
                _item = data->at(0);
                switch(_item.typeID){
                    case SINGLE_POINT_INFORMATION:
                        if(class1.size() < 20) addItem();
                        else _done = true;
                        break;
                    case DOUBLE_POINT_INFORMATION:
                        if(class1.size() < 20) addItem();
                        else _done = true;
                        break;
                    case STEP_POSITION_INFORMATION:
                        if (class1.size() < 18) addItem();
                        else _done = true;
                        break;
                    case NORMALIZED_VALUE:
                        if (class1.size() < 38) addItem();
                        else _done = true;
                        break;
                    case SCALED_VALUE:
                        if (class1.size() < 38) addItem();
                        else _done = true;
                        break;
                    case SHORT_FLOATING_POINT_NUMBER:
                        if (class1.size() < 28) addItem();
                        else _done = true;
                        break;
                    default:
                        break;
                }
            }
            else{
                addItem();
                _done = true;
            }
        }
        else if(data->size() > 0){
            addItem();
            _done = true;
        }
    }
}

void session::addItem(){
    pthread_mutex_lock(&_mtx);
    class1.push_back(data->at(0));
    data->erase(data->begin());
    pthread_mutex_unlock(&_mtx);
}

void session::spi2Class2(){
    class2.clear();
    if(spiSQ){
        if (spis->size() < 127 ){
            for (unsigned int i = 0; i < spis->size(); i++){
                S104::SPI spi = spis->at(i);
                class2.push_back(spi);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 127) && (i < spis->size()); i++){
                S104::SPI spi = spis->at(i);
                class2.push_back(spi);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (spis->size() < 60){
            for (unsigned int i = 0; i < spis->size(); i++){
                S104::SPI spi = spis->at(i);
                class2.push_back(spi);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 60) && (i < spis->size()); i++){
                S104::SPI spi = spis->at(i);
                class2.push_back(spi);
                j++;
            }
            ptr += j;
        }
    }
}

void session::dpi2Class2(){
    class2.clear();
    if(dpiSQ){
        if (dpis->size() < 127){
            for (unsigned int i = 0; i < dpis->size(); i++){
                S104::DPI dpi = dpis->at(i);
                class2.push_back(dpi);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 127) && (i < dpis->size()); i++){
                S104::DPI dpi = dpis->at(i);
                class2.push_back(dpi);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (dpis->size() < 60){
            for (unsigned int i  = 0; i < dpis->size(); i++){
                S104::DPI dpi = dpis->at(i);
                class2.push_back(dpi);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 60) && (i < dpis->size()); i++){
                S104::DPI dpi = dpis->at(i);
                class2.push_back(dpi);
                j++;
            }
            ptr += j;
        }
    }
}

void session::vti2Class2(){
    class2.clear();
    if(vtiSQ){
        if (vtis->size() < 121){
            for (unsigned int i = 0; i < vtis->size(); i++){
                S104::VTI vti = vtis->at(i);
                class2.push_back(vti);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 121) && (i < vtis->size()); i++){
                S104::VTI vti = vtis->at(i);
                class2.push_back(vti);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (vtis->size() < 48){
            for (unsigned int i = 0; i < vtis->size(); i++){
                S104::VTI vti = vtis->at(i);
                class2.push_back(vti);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 48) && (i < vtis->size()); i++){
                S104::VTI vti = vtis->at(i);
                class2.push_back(vti);
                j++;
            }
            ptr += j;
        }
    }
}

void session::nva2Class2(){
    class2.clear();
    if(nvaSQ){
        if (nvas->size() < 80){
            for (unsigned int i = 0; i < nvas->size(); i++){
                S104::NVA nva = nvas->at(i);
                class2.push_back(nva);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 80) && (i < nvas->size()); i++){
                S104::NVA nva = nvas->at(i);
                class2.push_back(nva);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (nvas->size() < 40){
            for (unsigned int i = 0; i < nvas->size(); i++){
                S104::NVA nva = nvas->at(i);
                class2.push_back(nva);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 40) && (i< nvas->size()); i++){
                S104::NVA nva = nvas->at(i);
                class2.push_back(nva);
                j++;
            }
            ptr += j;
        }
    }
}

void session::sva2Class2(){
    class2.clear();
    if (svaSQ){
        if (svas->size() < 80){
            for (unsigned int i = 0; i < svas->size(); i++){
                S104::SVA sva = svas->at(i);
                class2.push_back(sva);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 80) && (i < svas->size()); i++){
                S104::SVA sva = svas->at(i);
                class2.push_back(sva);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (svas->size() < 40){
            for (unsigned int i = 0; i < svas->size(); i++){
                S104::SVA sva = svas->at(i);
                class2.push_back(sva);
                ptr++;
            }
        }
        else{
            unsigned int j  = 0;
            for (unsigned int i = ptr; (i < ptr + 40) && (i < svas->size()); i++){
                S104::SVA sva = svas->at(i);
                class2.push_back(sva);
                j++;
            }
            ptr += j;
        }
    }
}

void session::fp2Class2(){
    class2.clear();
    if(fpSQ){
        if (fps->size() < 48){
            for (unsigned int i = 0; i < fps->size(); i++){
                S104::FP fp = fps->at(i);
                class2.push_back(fp);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 48) && (i < fps->size()); i++){
                S104::FP fp = fps->at(i);
                class2.push_back(fp);
                j++;
            }
            ptr += j;
        }
    }
    else{
        if (fps->size() < 30){
            for (unsigned int i = 0; i < fps->size(); i++){
                S104::FP fp = fps->at(i);
                class2.push_back(fp);
                ptr++;
            }
        }
        else{
            unsigned int j = 0;
            for (unsigned int i = ptr; (i < ptr + 30) && (i < fps->size()); i++){
                S104::FP fp = fps->at(i);
                class2.push_back(fp);
                j++;
            }
            ptr += j;
        }
    }
}

void session::EvaluateUSFormat(){
    switch(recvMsg[2]){
        case SUPERVISORY:
            recvSUPERVISORY();
            break;
        case STARTDT_act:
            recvSTARTDT_act();
            break;
        case STARTDT_con:
            recvSTARTDT_con();
            break;
        case STOPDT_act:
            recvSTOPDT_act();
            break;
        case STOPDT_con:
            recvSTOPDT_con();
            break;
        case TESTFR_act:
            recvTESTFR_act();
            break;
        case TESTFR_con:
            recvTESTFR_con();
            break;
    }
}

void session::EvaluateIFormat(){
    nr = ((((short)recvMsg[3]) << 8) | recvMsg[2]) >> 1;
    ns = ((((short)recvMsg[5]) << 8) | recvMsg[4]) >> 1;
    if(nr == NR){
        T1.stop();
        T3.stop();
        NR = short((NR + 1) & 0x7FFF);
        Count = Count - (ns - _ns);
        _ns = ns;
        if(debug->at(1) == true){
            stringstream _debug_stream;
            _debug_stream << "<=== : INFORMATION NS[" << (NR & 0x7FFF) << "] NR[" << ns << "]\n";
            if(debug_print_func != NULL){
                    debug_print_func(_debug_stream.str());
            }
            else cout << _debug_stream.str() << endl;
        }
        switch (recvMsg[6]){
            case SINGLE_COMMAND:
                recvSingleCommand();
                break;
            case DOUBLE_COMMAND:
                recvDoubleCommand();
                break;
            case REGULATING_STEP_COMMAND:
                recvRegulatingStepCommand();
                break;
            case SETPOINT_COMMAND_NORMALIZED_VALUE:
                recvSetPointCommandNormalizedValue();
                break;
            case SETPOINT_COMMAND_SCALED_VALUE:
                recvSetPointCommandScaledValue();
                break;
            case SETPOINT_COMMAND_SHORT_FLOATING_NUMBER:
                recvSetPointCommandShortFloating();
                break;
            case INTERROGATION_COMMAND:
                recvInterrogation();
                break;
            case CLOCK_SYNCHRONIZATION_COMMAND:
                recvClockSynchronization();
                break;
        }
    }
    else{
        isRunning = false;
    }
}

void session::UConfirm(){
    switch(reqUConfirm){
        case STARTDT_con:
            enableDT = true;
            sendSTARTDT_con();
            break;
        case STOPDT_con:
            enableDT = false;
            sendSTOPDT_con();
            break;
        case TESTFR_con:
            sendTESTFR_con();
            break;
    }
    reqUConfirm = 0;
}

void session::INTERConfirm(){
    if(Count < K){
        switch(reqINTERConfirm){
            case ACTIVATION_CONFIRM:
                if(spis->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = SINGLE_POINT_INFORMATION;
                }
                else if(dpis->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = DOUBLE_POINT_INFORMATION;
                }
                else if(vtis->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = STEP_POSITION_INFORMATION;
                }
                else if(nvas->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = NORMALIZED_VALUE;
                }
                else if(svas->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = NORMALIZED_VALUE;
                }
                else if(fps->size() > 0){
                    reqINTERConfirm = 0;
                    respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                }
                else reqINTERConfirm = ACTIVATION_TERMINATION;
                sendInterrogation(ACTIVATION_CONFIRM);
                break;
            case DEACTIVATION_CONFIRM:
                reqINTERConfirm = 0;
                sendInterrogation(DEACTIVATION_CONFIRM);
                break;
            case ACTIVATION_TERMINATION:
                reqINTERConfirm = 0;
                sendInterrogation(ACTIVATION_TERMINATION);
                break;
        }
    }
}

void session::SYNCConfirm(){
    if(Count < K){
        switch(reqSYNCConfirm){
            case ACTIVATION_CONFIRM:
                sendClockSynchronization(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case NEGATIVE_ACTIVATION_CONFIRM:
                sendClockSynchronization(NEGATIVE, ACTIVATION_CONFIRM);
                break;
        }
        reqSYNCConfirm = 0;
    }
}

void session::CTRLConfirm(){
    if(Count < K){
        switch(reqCTRLConfirm){
            case SINGLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM:
                sendSingleCommand(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM:
                sendSingleCommand(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case SINGLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM:
                sendSingleCommand(POSITIVE, DEACTIVATION_CONFIRM);
                break;
            case DOUBLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM:
                sendDoubleCommand(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM:
                sendDoubleCommand(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case DOUBLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM:
                sendDoubleCommand(POSITIVE, DEACTIVATION_CONFIRM);
                break;
            case REGULATING_STEP_COMMAND_POSITIVE_ACTIVATION_CONFIRM:
                sendRegulatingStepCommand(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM:
                sendRegulatingStepCommand(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case REGULATING_STEP_COMMAND_POSITIVE_DEACTIVATION_CONFIRM:
                sendRegulatingStepCommand(POSITIVE, DEACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandNormalizedValue(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandNormalizedValue(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_DEACTIVATION_CONFIRM:
                sendSetPointCommandScaledValue(POSITIVE, DEACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandScaledValue(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandScaledValue(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_DEACTIVATION_CONFIRM:
                sendSetPointCommandScaledValue(POSITIVE, DEACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandShortFloating(POSITIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM:
                sendSetPointCommandShortFloating(NEGATIVE, ACTIVATION_CONFIRM);
                break;
            case SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_DEACTIVATION_CONFIRM:
                sendSetPointCommandShortFloating(POSITIVE, DEACTIVATION_CONFIRM);
                break;
        }
        reqCTRLConfirm = 0;
    }
}

void session::class1Respond(){
    data2Class1();
    if(Count < K){
        switch(class1.at(0).typeID){
            case SINGLE_POINT_INFORMATION:
                sendSinglePointInformationWithDateTime(class1, SPONTANEOUS);
                break;
            case DOUBLE_POINT_INFORMATION:
                sendDoublePointInformationWithDateTime(class1, SPONTANEOUS);
                break;
            case STEP_POSITION_INFORMATION:
                sendStepPositionInformationWithDateTime(class1, SPONTANEOUS);
                break;
            case NORMALIZED_VALUE:
                sendMeasuredNormalizedValue(oSQ, class1, SPONTANEOUS);
                break;
            case SCALED_VALUE:
                sendMeasuredScaledValue(oSQ, class1, SPONTANEOUS);
                break;
            case SHORT_FLOATING_POINT_NUMBER:
                sendShortFloatingPoint(oSQ, class1, SPONTANEOUS);
                break;
            default:
                break;
        }
    }
}

void session::class2Respond(){
    if(tPeriodic.is_timeout()){
        if(nvas->size() > 0) respPERIODIC = NORMALIZED_VALUE;
        else if(svas->size() > 0) respPERIODIC = SCALED_VALUE;
        else if(fps->size() > 0) respPERIODIC = SHORT_FLOATING_POINT_NUMBER;
        tPeriodic.start();
    }
    if(tBackground.is_timeout()){
        if(spis->size() > 0) respBACKGROUND = SINGLE_POINT_INFORMATION;
        else if(dpis->size() > 0) respBACKGROUND = DOUBLE_POINT_INFORMATION;
        else if(vtis->size() > 0) respBACKGROUND = STEP_POSITION_INFORMATION;
        tBackground.start();
    }
    if(respINTERROGATE > 0) InterrogatedRespond();
    else if(respPERIODIC > 0) PeriodicRespond();
    else if(respBACKGROUND > 0) BackgroundScanRespond();
    else if(T3.is_timeout()) sendTESTFR_act();
}

void session::InterrogatedRespond(){
    while(respINTERROGATE > 0){
        if(Count < K){
            switch(respINTERROGATE){
                case SINGLE_POINT_INFORMATION:
                    spi2Class2();
                    if(ptr >= spis->size()){
                        ptr = 0;
                        if(dpis->size() > 0) respINTERROGATE = DOUBLE_POINT_INFORMATION;
                        else if(vtis->size() > 0) respINTERROGATE = STEP_POSITION_INFORMATION;
                        else if(nvas->size()> 0) respINTERROGATE = NORMALIZED_VALUE;
                        else if(svas->size() > 0) respINTERROGATE = SCALED_VALUE;
                        else if(fps->size() > 0) respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                        else{
                            respINTERROGATE = 0;
                            reqINTERConfirm = ACTIVATION_TERMINATION;
                        }
                    }
                    sendSinglePointInformation(spiSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
                case DOUBLE_POINT_INFORMATION:
                    dpi2Class2();
                    if(ptr >= dpis->size()){
                        ptr = 0;
                        if(vtis->size() > 0) respINTERROGATE = STEP_POSITION_INFORMATION;
                        else if(nvas->size() > 0) respINTERROGATE = NORMALIZED_VALUE;
                        else if(svas->size() > 0) respINTERROGATE = SCALED_VALUE;
                        else if(fps->size() > 0) respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                        else{
                            respINTERROGATE  = 0;
                            reqINTERConfirm = ACTIVATION_TERMINATION;
                        }
                    }
                    sendDoublePointInformation(dpiSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
                case STEP_POSITION_INFORMATION:
                    vti2Class2();
                    if(ptr >= vtis->size()){
                        ptr = 0;
                        if(nvas->size() > 0) respINTERROGATE = NORMALIZED_VALUE;
                        else if(svas->size() > 0) respINTERROGATE = SCALED_VALUE;
                        else if(fps->size() > 0) respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                        else{
                            respINTERROGATE = 0;
                            reqINTERConfirm = ACTIVATION_TERMINATION;
                        }
                    }
                    sendStepPositionInformation(vtiSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
                case NORMALIZED_VALUE:
                    nva2Class2();
                    if (ptr >= nvas->size()){
                        ptr = 0;
                        if(svas->size() > 0) respINTERROGATE = SCALED_VALUE;
                        else if(fps->size() > 0) respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                        else{
                            respINTERROGATE = 0;
                            reqINTERConfirm = ACTIVATION_TERMINATION;
                        }
                    }
                    sendMeasuredNormalizedValue(nvaSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
                case SCALED_VALUE:
                    sva2Class2();
                    if(ptr >= svas->size()){
                        ptr = 0;
                        if(fps->size() > 0) respINTERROGATE = SHORT_FLOATING_POINT_NUMBER;
                        else{
                            respINTERROGATE = 0;
                            reqINTERConfirm = ACTIVATION_TERMINATION;
                        }
                    }
                    sendMeasuredScaledValue(svaSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
                case SHORT_FLOATING_POINT_NUMBER:
                    fp2Class2();
                    if(ptr >= fps->size()){
                        ptr = 0;
                        respINTERROGATE = 0;
                        reqINTERConfirm = ACTIVATION_TERMINATION;
                    }
                    sendShortFloatingPoint(fpSQ, class2, INTERROGATED_BY_STATION_INTERROGATION);
                    break;
            }
        }
    }
}

void session::PeriodicRespond(){
    while(respPERIODIC > 0){
        if(Count < K){
            switch(respPERIODIC){
                case NORMALIZED_VALUE:
                    nva2Class2();
                    if(ptr >= nvas->size()){
                        ptr = 0;
                        if(svas->size() > 0) respPERIODIC = SCALED_VALUE;
                        else if(fps->size() > 0) respPERIODIC = SHORT_FLOATING_POINT_NUMBER;
                        else respPERIODIC = 0;
                    }
                    sendMeasuredNormalizedValue(nvaSQ, class2, PERIODIC);
                    break;
                case SCALED_VALUE:
                    sva2Class2();
                    if(ptr >= svas->size()){
                        ptr = 0;
                        if(fps->size() > 0) respPERIODIC = SHORT_FLOATING_POINT_NUMBER;
                        else respPERIODIC = 0;
                    }
                    sendMeasuredScaledValue(svaSQ, class2, PERIODIC);
                    break;
                case SHORT_FLOATING_POINT_NUMBER:
                    fp2Class2();
                    if(ptr >= fps->size()){
                        ptr = 0;
                        respPERIODIC = 0;
                    }
                    sendShortFloatingPoint(fpSQ, class2, PERIODIC);
                    break;
            }
        }
    }
}

void session::BackgroundScanRespond(){
    while(respBACKGROUND > 0){
        if(Count < K){
            switch(respBACKGROUND){
                case SINGLE_POINT_INFORMATION:
                    spi2Class2();
                    if(ptr >= spis->size()){
                        ptr = 0;
                        if(dpis->size() > 0) respBACKGROUND = DOUBLE_POINT_INFORMATION;
                        else if(vtis->size() > 0) respBACKGROUND = STEP_POSITION_INFORMATION;
                        else respBACKGROUND = 0;
                    }
                    sendSinglePointInformation(spiSQ, class2, BACKGROUND_SCAN);
                    break;
                case DOUBLE_POINT_INFORMATION:
                    dpi2Class2();
                    if(ptr >= dpis->size()){
                        ptr = 0;
                        if(vtis->size() > 0) respBACKGROUND = STEP_POSITION_INFORMATION;
                        else respBACKGROUND = 0;
                    }
                    sendDoublePointInformation(dpiSQ, class2, BACKGROUND_SCAN);
                    break;
                case 5:
                    vti2Class2();
                    if(ptr >= vtis->size()){
                        ptr = 0;
                        respBACKGROUND = 0;
                    }
                    sendStepPositionInformation(vtiSQ, class2, BACKGROUND_SCAN);
                    break;
            }
        }
    }
}

void session::recvSTARTDT_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : STARTDT act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    tPeriodic.start();
    tBackground.start();
    reqUConfirm = STARTDT_con;
}

void session::recvSTARTDT_con(){
    T1.stop();
    T3.start();
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : STARTDT con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
}

void session::recvSTOPDT_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : STOPDT act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    reqUConfirm = STOPDT_con;
}

void session::recvSTOPDT_con(){
    T1.stop();
    T3.start();
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : STOPDT con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
}

void session::recvTESTFR_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : TESTFR act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    reqUConfirm = TESTFR_con;
}

void session::recvTESTFR_con(){
    T1.stop();
    T3.start();
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "<=== : TESTFR con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
}

void session::recvSUPERVISORY(){
    T1.stop();
    T3.start();
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream <<  "<=== : SUPERVISORY NR[" << ns << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    ns = ((((short)recvMsg[5]) << 8) | recvMsg[4]) >> 1;
    Count -= (ns - _ns);
    _ns = ns;
}

void session::recvInterrogation(){
    _cot = recvMsg[8] & 0x3F;
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Interrogation Command Count[1] COT[" << _cot << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch (_cot){
        case ACTIVATION:
            respPERIODIC = 0;
            respBACKGROUND = 0;
            reqINTERConfirm = ACTIVATION_CONFIRM;
            break;
        case DEACTIVATION:
            reqINTERConfirm = DEACTIVATION_CONFIRM;
            break;
    }
}

void session::recvClockSynchronization(){
    _cot = recvMsg[8] & 0x3F;
    timeval _tm;
    int _milisecond, _second, _minute, _hour, _day, _month, _year;
    _milisecond = 1000*((recvMsg[16] << 8 | recvMsg[15]) % 1000);
    _second = (recvMsg[16] << 8 | recvMsg[15]) / 1000;
    _minute = (recvMsg[17] & 0x3F);
    _hour = (recvMsg[18] & 0x1F);
    _day = (recvMsg[19] & 0x1F);
    _month = (recvMsg[20] & 0x0F);
    _year = (recvMsg[21] & 0x1F);
    gettimeofday(&_tm, NULL);
    tm *_time = localtime(&_tm.tv_sec);
    _time->tm_sec = _second;
    _time->tm_min = _minute;
    _time->tm_hour = _hour;
    _time->tm_mday = _day;
    _time->tm_mon = _month - 1;
    _time->tm_year = _year + 100;
    _tm.tv_sec = mktime(_time);
    _tm.tv_usec = _milisecond;
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Clock Synchronization Command Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Time[" << Converter::ToString(_tm).c_str() << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if (settimeofday(&_tm, NULL) < 0) reqSYNCConfirm = NEGATIVE_ACTIVATION_CONFIRM;
    else reqSYNCConfirm = ACTIVATION_CONFIRM;
}

void session::recvSingleCommand(){
    _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[15] & 0x80);
    _QOC = recvMsg[15] & 0x3C;
    _Value[0] = recvMsg[15] & 0x01;
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Single Command Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Address[" << __objectAddress << "] SE[" << _se << "] Value[" << _Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == SINGLE_COMMAND){
                    if(isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = SINGLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value = Converter::ToString((int)_Value[0]);
                                if (update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SINGLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                        }
                        else{
                            if(_objectAddress == _objectAddress){
                                string _value = Converter::ToString((int)_Value[0]);
                                if (update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SINGLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = SINGLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = SINGLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM;
            break;
    }
}

void session::recvDoubleCommand(){
     _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[15] & 0x80);
    _QOC = recvMsg[15] & 0x3C;
    _Value[0] = recvMsg[15] & 0x03;
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Double Command Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Address[" << __objectAddress << "] SE[" << _se << "] Value[" << _Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == DOUBLE_COMMAND){
                    if(isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = DOUBLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value;
                                if(control->at(_index).ref2 == ""){
                                    _value = Converter::ToString((int)_Value[0]);
                                    if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                }
                                else{
                                    switch(_Value[0]){
                                        case 1:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, "1");
                                            break;
                                        case 2:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref2, "1");
                                            break;
                                    }
                                }
                                reqCTRLConfirm = DOUBLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                        }
                        else{
                            if(_objectAddress == __objectAddress){
                                string _value;
                                if(control->at(_index).ref2 == ""){
                                    _value = Converter::ToString((int)_Value[0]);
                                    if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                }
                                else{
                                    switch(_Value[0]){
                                        case 1:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, "1");
                                            break;
                                        case 2:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref2, "1");
                                            break;
                                    }
                                }
                                reqCTRLConfirm = DOUBLE_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = DOUBLE_COMMAND_NEGATIVE_ACTIVATION_CONFIRM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = DOUBLE_COMMAND_POSITIVE_DEACTIVATION_CONFIRM;
            break;
    }
}

void session::recvRegulatingStepCommand(){
     _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[15] & 0x80);
    _QOC = recvMsg[15] & 0x3C;
    _Value[0] = recvMsg[15] & 0x03;
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Regulating Step Command Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Address[" << __objectAddress << "] SE[" << _se << "] Value[" << _Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == REGULATING_STEP_COMMAND){
                    if (isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value;
                                if(control->at(_index).ref2 == ""){
                                    _value = Converter::ToString((int)_Value[0]);
                                    if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                }
                                else{
                                    switch(_Value[0]){
                                        case 1:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, "1");
                                            break;
                                        case 2:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref2, "1");
                                            break;
                                    }
                                }
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
                        }
                        else{
                            if(_objectAddress == __objectAddress){
                                string _value;
                                if(control->at(_index).ref2 == ""){
                                    _value = Converter::ToString((int)_Value[0]);
                                    if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                }
                                else{
                                    switch(_Value[0]){
                                        case 1:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, "1");
                                            break;
                                        case 2:
                                            if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref2, "1");
                                            break;
                                    }
                                }
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = REGULATING_STEP_COMMAND_NEGATIVE_ACTIVATION_CONFRIM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = REGULATING_STEP_COMMAND_POSITIVE_DEACTIVATION_CONFIRM;
            break;
    }
}

void session::recvSetPointCommandNormalizedValue(){
     _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[17] & 0x80);
    _QOC = 0x00;
    _Value[0] = recvMsg[15];
    _Value[1] = recvMsg[16];
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Set-point Command, Normalized Value Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" <<  _se
                      << "] Value[" << (short)(_Value[1] << 8 | _Value[0]) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == SETPOINT_COMMAND_NORMALIZED_VALUE){
                    if(isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value = Converter::ToString((short)(_Value[1] << 8 | _Value[0]));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                        }
                        else{
                            if(_objectAddress == __objectAddress){
                                string _value = Converter::ToString((short)(_Value[1] << 8 | _Value[0]));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = SETPOINT_COMMAND_NORMALIZED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
            break;
    }
}

void session::recvSetPointCommandScaledValue(){
     _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[17] & 0x80);
    _QOC = 0;
    _Value[0] = recvMsg[15];
    _Value[1] = recvMsg[16];
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Set-point Command, Scaled Value Count[1] COT[" << _cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" <<  _se
                      << "] Value[" << (short)(_Value[1] << 8 | _Value[0]) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == SETPOINT_COMMAND_SCALED_VALUE){
                    if(isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value = Converter::ToString((short)(_Value[1] << 8 | _Value[0]));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                        }
                        else{
                            if(_objectAddress == __objectAddress){
                                string _value = Converter::ToString((int)(_Value[1] << 8 | _Value[0]));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_NEGATIVE_ACTIVATION_CONFIRM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = SETPOINT_COMMAND_SCALED_VALUE_POSITIVE_DEACTIVATION_CONFIRM;
            break;
    }
}

void session::recvSetPointCommandShortFloating(){
     _cot = recvMsg[8] & 0x3F;
    __objectAddress = (recvMsg[14] << 16 | recvMsg[13] << 8 | recvMsg[12]);
    _se = SE(recvMsg[19] & 0x80);
    _QOC = 0;
    _Value[0] = recvMsg[15];
    _Value[1] = recvMsg[16];
    _Value[2] = recvMsg[17];
    _Value[3] = recvMsg[18];
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "<~~~ : Set-point Command, Short Floating Point Count[1] COT[" << _cot <<"]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se << std::setprecision(3)
                      << "] Value[" << Converter::ToFloat(_Value) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    switch(_cot){
        case ACTIVATION:
            if(ctl.count(__objectAddress) > 0){
                int _index = ctl[__objectAddress];
                if(control->at(_index).typeID == SETPOINT_COMMAND_SHORT_FLOATING_NUMBER){
                    if(isControlling == 0){
                        if(_se == SELECT){
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                isControlling = 1;
                                tSBO.start();
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                        else{
                            if(control->at(_index).sbo){
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                string _value = Converter::ToString(Converter::ToFloat(_Value));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                    else{
                        isControlling = 0;
                        if(_se == SELECT){
                            _objectAddress = __objectAddress;
                            reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
                        }
                        else{
                            if(_objectAddress == __objectAddress){
                                string _value = Converter::ToString(Converter::ToFloat(_Value));
                                if(update_to_dnp != NULL) update_to_dnp(control->at(_index).ref1, _value);
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_ACTIVATION_CONFIRM;
                            }
                            else{
                                _objectAddress = __objectAddress;
                                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
                            }
                        }
                    }
                }
                else{
                    _objectAddress = __objectAddress;
                    reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
                }
            }
            else{
                _objectAddress = __objectAddress;
                reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_NEGATIVE_ACTIVATION_CONFIRM;
            }
            break;
        case DEACTIVATION:
            _objectAddress = __objectAddress;
            isControlling = 0;
            reqCTRLConfirm = SETPOINT_COMMAND_SHORT_FLOATING_NUMBER_POSITIVE_DEACTIVATION_CONFIRM;
            break;
    }
}

void session::sendSTARTDT_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream <<  "===> : STARTD act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x07;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
}

void session::sendSTARTDT_con(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : STARTDT con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x0B;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
}

void session::sendSTOPDT_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : STOPDT act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x13;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
}

void session::sendSTOPDT_con(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : STOPDT con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x23;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
}

void session::sendTESTFR_act(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : TESTFR act\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str();
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x43;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
}

void session::sendTESTFR_con(){
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : TESTFR con\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    sendLength = 6;
    sendMsg[0] = 0x68;
    sendMsg[1] = 0x04;
    sendMsg[2] = 0x83;
    sendMsg[3] = 0x00;
    sendMsg[4] = 0x00;
    sendMsg[5] = 0x00;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
}

void session::sendInterrogation(COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Interrogation Command Count[1] COT[" << cot << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    sendLength = 16;
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x64;
    sendMsg[7] = 0x01;
    sendMsg[8] = cot;
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = 0x00;
    sendMsg[13] = 0x00;
    sendMsg[14] = 0x00;
    sendMsg[15] = 0x14;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendClockSynchronization(PN pn, COT cot){
    timeval _tm;
    gettimeofday(&_tm, NULL);
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Clock Synchronization Command Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Time[" << Converter::ToString(_tm).c_str() << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _time[7];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::TimeToBytes(_time, _tm);
    sendLength = 22;
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x67;
    sendMsg[7] = 0x01;
    sendMsg[8] = (pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = 0x00;
    sendMsg[13] = 0x00;
    sendMsg[14] = 0x00;
    sendMsg[15] = _time[0];
    sendMsg[16] = _time[1];
    sendMsg[17] = _time[2];
    sendMsg[18] = _time[3];
    sendMsg[19] = _time[4];
    sendMsg[20] = _time[5];
    sendMsg[21] = _time[6];
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSinglePointInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Single Point Information SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x01;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = char(cot);
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + i] = (_item.value[0] | _item.quality) & 0xF1;
        }
    }
    else{
        sendLength = 12 + 4 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x01;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 4 * i] = _address[0];
            sendMsg[13 + 4 * i] = _address[1];
            sendMsg[14 + 4 * i] = _address[2];
            sendMsg[15 + 4 * i] = (_item.value[0] | _item.quality) & 0xF1;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSinglePointInformationWithDateTime(vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Single Point Information SQ[0] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality
                          << "] Time[" << Converter::ToString(items[i].time) << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    sendLength = 12 + 11 * items.size();
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x1E;
    sendMsg[7] = (items.size() & 0x7F);
    sendMsg[8] = cot;
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    for (unsigned int i = 0; i < items.size(); i++)
    {
        S104::infItem _item = items.at(i);
        char _address[4];
        char _time[7];
        BitConverter::IntToBytes(_address, _item.address);
        BitConverter::TimeToBytes(_time, _item.time);
        sendMsg[12 + 11 * i] = _address[0];
        sendMsg[13 + 11 * i] = _address[1];
        sendMsg[14 + 11 * i] = _address[2];
        sendMsg[15 + 11 * i] = (_item.value[0] | _item.quality) & 0xF1;
        sendMsg[16 + 11 * i] = _time[0];
        sendMsg[17 + 11 * i] = _time[1];
        sendMsg[18 + 11 * i] = _time[2];
        sendMsg[19 + 11 * i] = _time[3];
        sendMsg[20 + 11 * i] = _time[4];
        sendMsg[21 + 11 * i] = _time[5];
        sendMsg[22 + 11 * i] = _time[6];
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendDoublePointInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Single Point Information SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x03;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = char(cot);
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + i] = (_item.value[0] | _item.quality) & 0xF3;
        }
    }
    else{
        sendLength = 12 + 4 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x03;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 4 * i] = _address[0];
            sendMsg[13 + 4 * i] = _address[1];
            sendMsg[14 + 4 * i] = _address[2];
            sendMsg[15 + 4 * i] = (_item.value[0] | _item.quality) & 0xF3;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendDoublePointInformationWithDateTime(vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Double Point Information SQ[0] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality
                          << "] Time[" << Converter::ToString(items[i].time) << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    sendLength = 12 + 11 * items.size();
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x1F;
    sendMsg[7] = (items.size() & 0x7F);
    sendMsg[8] = cot;
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    for (unsigned int i = 0; i < items.size(); i++)
    {
        S104::infItem _item = items.at(i);
        char _address[4];
        char _time[7];
        BitConverter::IntToBytes(_address, _item.address);
        BitConverter::TimeToBytes(_time, _item.time);
        sendMsg[12 + 11 * i] = _address[0];
        sendMsg[13 + 11 * i] = _address[1];
        sendMsg[14 + 11 * i] = _address[2];
        sendMsg[15 + 11 * i] = (_item.value[0] | _item.quality) & 0xF3;
        sendMsg[16 + 11 * i] = _time[0];
        sendMsg[17 + 11 * i] = _time[1];
        sendMsg[18 + 11 * i] = _time[2];
        sendMsg[19 + 11 * i] = _time[3];
        sendMsg[20 + 11 * i] = _time[4];
        sendMsg[21 + 11 * i] = _time[5];
        sendMsg[22 + 11 * i] = _time[6];
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendStepPositionInformation(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Step Position Information SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + 2 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x05;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + 2 * i] = _item.value[0] & 0x7F;
            sendMsg[16 + 2 * i] = _item.quality & 0xF1;
        }
    }
    else{
        sendLength = 12 + 5 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x05;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 5 * i] = _address[0];
            sendMsg[13 + 5 * i] = _address[1];
            sendMsg[14 + 5 * i] = _address[2];
            sendMsg[15 + 5 * i] = _item.value[0] & 0x7F;
            sendMsg[16 + 5 * i] = _item.quality & 0xF1;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendStepPositionInformationWithDateTime(vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Step Position Information SQ[0] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)items[i].value[0]
                          << "] Quality[" << (int)items[i].quality
                          << "] Time[" << Converter::ToString(items[i].time) << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    sendLength = 12 + 12 * items.size();
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x20;
    sendMsg[7] = (items.size() & 0x7F);
    sendMsg[8] = cot;
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    for (unsigned int i = 0; i < items.size(); i++)
    {
        S104::infItem _item = items.at(i);
        char _address[4];
        char _time[7];
        BitConverter::IntToBytes(_address, _item.address);
        BitConverter::TimeToBytes(_time, _item.time);
        sendMsg[12 + 12 * i] = _address[0];
        sendMsg[13 + 12 * i] = _address[1];
        sendMsg[14 + 12 * i] = _address[2];
        sendMsg[15 + 12 * i] = _item.value[0] & 0x7F;
        sendMsg[16 + 12 * i] = _item.quality & 0xF1;
        sendMsg[17 + 12 * i] = _time[0];
        sendMsg[18 + 12 * i] = _time[1];
        sendMsg[19 + 12 * i] = _time[2];
        sendMsg[20 + 12 * i] = _time[3];
        sendMsg[21 + 12 * i] = _time[4];
        sendMsg[22 + 12 * i] = _time[5];
        sendMsg[23 + 12 * i] = _time[6];
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendMeasuredNormalizedValue(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Measured Normalized Value SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)(items[i].value[1] << 8 | items[i].value[0])
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::IntToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + 3 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x09;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + 3 * i] = _item.value[0];
            sendMsg[16 + 3 * i] = _item.value[1];
            sendMsg[17 + 3 * i] = _item.quality & 0xF1;
        }
    }
    else{
        sendLength = 12 + 6 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x09;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 6 * i] = _address[0];
            sendMsg[13 + 6 * i] = _address[1];
            sendMsg[14 + 6 * i] = _address[2];
            sendMsg[15 + 6 * i] = _item.value[0];
            sendMsg[16 + 6 * i] = _item.value[1];
            sendMsg[17 + 6 * i] = _item.quality & 0xF1;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendMeasuredScaledValue(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Measured Scaled Value SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address
                          << "] Value[" << (int)(items[i].value[1] << 8 | items[i].value[0])
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + 3 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x0B;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + 3 * i] = _item.value[0];
            sendMsg[16 + 3 * i] = _item.value[1];
            sendMsg[17 + 3 * i] = _item.quality & 0xF1;
        }
    }
    else{
        sendLength = 12 + 6 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x0B;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 6 * i] = _address[0];
            sendMsg[13 + 6 * i] = _address[1];
            sendMsg[14 + 6 * i] = _address[2];
            sendMsg[15 + 6 * i] = _item.value[0];
            sendMsg[16 + 6 * i] = _item.value[1];
            sendMsg[17 + 6 * i] = _item.quality & 0xF1;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendShortFloatingPoint(SEQUENCE sq, vector<S104::infItem> items, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Measured Short Floating Point SQ[" << SQ << "] Count[" << items.size() << "] COT[" << cot << "]\n";
        for(unsigned int i = 0; i < items.size(); i++){
            _debug_stream << "       Address[" << items[i].address << std::setprecision(3)
                          << "] Value[" << Converter::ToFloat(items[i].value)
                          << "] Quality[" << (int)items[i].quality << "]\n";
        }
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    if (sq){
        sendLength = 15 + 5 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x0D;
        sendMsg[7] = (items.size() | 0x80);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        char _address[4];
        BitConverter::IntToBytes(_address, items.at(0).address);
        sendMsg[12] = _address[0];
        sendMsg[13] = _address[1];
        sendMsg[14] = _address[2];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            sendMsg[15 + 5 * i] = _item.value[0];
            sendMsg[16 + 5 * i] = _item.value[1];
            sendMsg[17 + 5 * i] = _item.value[2];
            sendMsg[18 + 5 * i] = _item.value[3];
            sendMsg[19 + 5 * i] = _item.quality & 0xF1;
        }
    }
    else{
        sendLength = 12 + 8 * items.size();
        sendMsg[0] = 0x68;
        sendMsg[1] = (sendLength - 2) & 0xFF;
        sendMsg[2] = _NS[0];
        sendMsg[3] = _NS[1];
        sendMsg[4] = _NR[0];
        sendMsg[5] = _NR[1];
        sendMsg[6] = 0x0D;
        sendMsg[7] = (items.size() & 0x7F);
        sendMsg[8] = cot;
        sendMsg[9] = 0x00;
        sendMsg[10] = _asduAddress[0];
        sendMsg[11] = _asduAddress[1];
        for (unsigned int i = 0; i < items.size(); i++)
        {
            S104::infItem _item = items.at(i);
            char _address[4];
            BitConverter::IntToBytes(_address, _item.address);
            sendMsg[12 + 8 * i] = _address[0];
            sendMsg[13 + 8 * i] = _address[1];
            sendMsg[14 + 8 * i] = _address[2];
            sendMsg[15 + 8 * i] = _item.value[0];
            sendMsg[16 + 8 * i] = _item.value[1];
            sendMsg[17 + 8 * i] = _item.value[2];
            sendMsg[18 + 8 * i] = _item.value[3];
            sendMsg[19 + 8 * i] = _item.quality & 0xF1;
        }
    }
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSingleCommand(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Single Command Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se
                      << "] Value[" << (int)_Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 16;
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x2D;
    sendMsg[7] = 0x01;
    sendMsg[8] = (pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = (_se | _QOC | _Value[0]);
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendDoubleCommand(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Double Command Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se
                      << "] Value[" << (int)_Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 16;
    sendMsg[0] = 0x68;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x2E;
    sendMsg[7] = 0x01;
    sendMsg[8] =(pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = (_se | _QOC | _Value[0]);
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendRegulatingStepCommand(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Regulating Step Command Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se
                      << "] Value[" << (int)_Value[0] << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 16;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x2F;
    sendMsg[7] = 0x01;
    sendMsg[8] = ( pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = (_se | _QOC | _Value[0]);
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSetPointCommandNormalizedValue(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Set-point Command, Normalized Value Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se
                      << "] Value[" << (short)(_Value[1] << 8 | _Value[0]) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 18;
    sendMsg[1] = (sendLength - 2)  & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x30;
    sendMsg[7] = 0x01;
    sendMsg[8] = (pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = _Value[0];
    sendMsg[16] = _Value[1];
    sendMsg[17] = _se;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSetPointCommandScaledValue(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Set-point Command, Scaled Value Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" << _se
                      << "] Value[" << (short)(_Value[1] << 8 | _Value[0]) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 18;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x31;
    sendMsg[7] = 0x01;
    sendMsg[8] = (pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = _Value[0];
    sendMsg[16] = _Value[1];
    sendMsg[17] = _se;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

void session::sendSetPointCommandShortFloating(PN pn, COT cot){
    if(debug->at(2) == true){
        stringstream _debug_stream;
        _debug_stream << "~~~> : Set-point Command, Short Floating Point Count[1] PN[" << pn << "] COT[" << cot << "]\n";
        _debug_stream << "       Address[" << _objectAddress
                      << "] SE[" <<  _se << std::setprecision(3)
                      << "] Value[" << Converter::ToFloat(_Value) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    if(debug->at(1) == true){
        stringstream _debug_stream;
        _debug_stream << "===> : INFORMATION NS[" << (NS & 0x7FFF) << "] NR[" << (NR & 0x7FFF) << "]\n";
        if(debug_print_func != NULL){
            debug_print_func(_debug_stream.str());
        }
        else cout << _debug_stream.str() << endl;
    }
    char _NS[2];
    char _NR[2];
    char _asduAddress[2];
    char _address[4];
    BitConverter::ShortToBytes(_NS, NS << 1);
    BitConverter::ShortToBytes(_NR, NR << 1);
    BitConverter::ShortToBytes(_asduAddress, asduAddress);
    BitConverter::IntToBytes(_address, _objectAddress);
    sendLength = 20;
    sendMsg[1] = (sendLength - 2) & 0xFF;
    sendMsg[2] = _NS[0];
    sendMsg[3] = _NS[1];
    sendMsg[4] = _NR[0];
    sendMsg[5] = _NR[1];
    sendMsg[6] = 0x32;
    sendMsg[7] = 0x01;
    sendMsg[8] = (pn | cot);
    sendMsg[9] = 0x00;
    sendMsg[10] = _asduAddress[0];
    sendMsg[11] = _asduAddress[1];
    sendMsg[12] = _address[0];
    sendMsg[13] = _address[1];
    sendMsg[14] = _address[2];
    sendMsg[15] = _Value[0];
    sendMsg[16] = _Value[1];
    sendMsg[17] = _Value[2];
    sendMsg[18] = _Value[3];
    sendMsg[19] = _se;
    layer1debug(false, sendMsg, sendLength);
    send(sock, sendMsg, sendLength, 0);
    T1.start();
    T3.stop();
    NS = short((NS + 1) & 0x7FFF);
    Count++;
}

}
