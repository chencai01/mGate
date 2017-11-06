#include "s104/server.h"
#include "s104/s104.h"
#include "s104/util.h"
#include "signal.h"
#include "errno.h"
#include "string.h"

using namespace std;
using namespace S104;

namespace S104{

server::server() {
    this->spiSQ = SQ;
    this->dpiSQ = SQ;
    this->vtiSQ = SQ;
    this->nvaSQ = SQ;
    this->svaSQ = SQ;
    this->fpSQ = SQ;
    this->is_running = false;
    this->update_to_dnp = NULL;

    pthread_mutex_init(&mtx_lock, NULL);

    debug.push_back(false);
    debug.push_back(false);
    debug.push_back(false);
}

server::~server(){
    pthread_mutex_destroy(&mtx_lock);
}

void* server::run_thread_functor(void* obj){
    server* __obj = (server*)obj;
    __obj->run();
    return NULL;
}

void* server::rand_thread_functor(void* obj){
    server* __obj = (server*)obj;
    __obj->random();
    return NULL;
}

void server::start(){
    pthread_create(&run_thread, NULL,
                   &server::run_thread_functor,
                   (void*)this);
}

void server::stop(){
    if(!is_running) return;
    is_running = false;
    for(int i = 0; i < MaxClient; i++){
        sessions[i].isRunning = false;
    }
    shutdown(serverSock, 2);
    pthread_join(run_thread, NULL);
    sleep(1);
    cout << "[IEC104] Server stopped!\n";
}

int server::init(){
    if(MaxClient >= MAX_CLIENT) MaxClient = MAX_CLIENT;
    if(MaxClient <=0) MaxClient = 1;
    //Init serverSock and start listen()'ing
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) return 1;
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    //For setsock opt (REUSEADDR)
    int yes = 1;

    //Avoid bind error if the socket was not close()'d last time;
    setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
    if(bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(sockaddr_in)) < 0) return 1;
    return 0;
}

void server::run(){
    listen(serverSock, 1);
    socklen_t cliSize = sizeof(sockaddr_in);
    if(is_running) return;
    is_running = true;
    cout << "[IEC104] Server running!\n";
    for (int i = 0; i < MaxClient; i++)
    {
        initSession(&sessions[i]);
    }
    while(is_running){
        if(getConnectedClient()<MaxClient){
            int sock = accept(serverSock, (struct sockaddr *) &clientAddr, &cliSize);
            for (int i = 0; i < MaxClient; i++)
            {
                if(sessions[i].sock < 0)
                {
                    sessions[i].sock = sock;
                    sessions[i].start();
                    break;
                }
            }
        }
        sleep(1);
    }
}

void server::random(){
    int _typeID, _index, _value;
    string _svalue;
    while(1){
        usleep(100000);
        _typeID = rand() % 6;
        switch(_typeID){
            case 0:
                if(spis.size() > 0){
                    _index = rand() % spis.size();
                    _value = rand() % 2;
                    _svalue = Converter::ToString(_value);
                    if(spis.at(_index).update(_svalue, 0x00) > 0){
                        S104::SPI _spi = spis.at(_index);
                        for(int i = 0; i < MaxClient; i++){
                            sessions[i].update(_spi);
                        }
                    }
                }
                break;
            case 1:
                if(dpis.size() > 0){
                    _index = rand() % dpis.size();
                    _value = rand() % 4;
                    _svalue = Converter::ToString(_value);
                    if(dpis.at(_index).update(_svalue, 0x00) > 0){
                        S104::DPI _dpi = dpis.at(_index);
                        for(int i = 0; i < MaxClient; i++){
                            sessions[i].update(_dpi);
                        }
                    }
                }
                break;
            case 2:
                if(vtis.size() > 0){
                    _index = rand() % vtis.size();
                    _value = rand() % 97;
                    _svalue = Converter::ToString(_value);
                    if(vtis.at(_index).update(_svalue, 0x00) > 0){
                        S104::VTI _vti = vtis.at(_index);
                        for(int i = 0; i < MaxClient; i++){
                            sessions[i].update(_vti);
                        }
                    }
                }
                break;
            case 3:
                if(nvas.size() > 0){
                    _index = rand() % nvas.size();
                    _value = rand() % 32768;
                    _svalue = Converter::ToString(_value);
                    nvas.at(_index).update(_svalue, 0x00);
                    S104::NVA _nva = nvas.at(_index);
                    for(int i  = 0; i < MaxClient; i++){
                        sessions[i].update(_nva);
                    }
                }
            case 4:
                break;
            case 5:
                if(fps.size() > 0){
                    _index = rand() % fps.size();
                    _value = rand() % 1000;
                    _svalue = Converter::ToString(_value);
                    fps.at(_index).update(_svalue, 0x00);
                    S104::FP _fp = fps.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_fp);
                    }
                }
                break;
        }
    }
}

int server::getConnectedClient(){
    int __count = 0;
    for(int i = 0; i < MaxClient; i++){
        if(sessions[i].sock > 0) __count++;
    }
    return __count;
}

void server::add_update_callback(update_dnp pfunc){
    update_to_dnp = pfunc;
}

void server::add_debug_callback(debug_print pfunc){
    debug_print_funct = pfunc;
}

void server::initSession(session *s){
    s->asduAddress = this->asduAddress;
    s->K = this->K;
    s->W = this->W;
    s->debug = &this->debug;
    s->spis = &this->spis;
    s->dpis = &this->dpis;
    s->vtis = &this->vtis;
    s->nvas = &this->nvas;
    s->svas = &this->svas;
    s->fps = &this->fps;
    s->control = &this->control;
    s->ctl = this->ctl;
    s->spiSQ = this->spiSQ;
    s->dpiSQ = this->dpiSQ;
    s->vtiSQ = this->vtiSQ;
    s->nvaSQ = this->nvaSQ;
    s->svaSQ = this->svaSQ;
    s->fpSQ = this->fpSQ;
    s->T0 = this->T0;
    s->T1.set_time(this->T1);
    s->T2.set_time(this->T2);
    s->T3.set_time(this->T3);
    s->tSBO.set_time(this->SBOTimeout);
    s->tPeriodic.set_time(this->Periodic);
    s->tBackground.set_time(this->Background);
    s->add_pfunc(update_to_dnp);
    s->add_pfunc(debug_print_funct);
}

void server::update(string _ref, string _value, char _quality){
    string _key;
    if(this->inf.count(_ref) > 0)
    {
        _key = this->inf[_ref];
        vector<string> _tokens;
        splitString(_tokens, _key, '.');
        int _typeID = atoi(_tokens[0].c_str());
        int _index = atoi(_tokens[1].c_str());
        switch(_typeID)
        {
            case 1:
                if(spis.at(_index).update(_value, _quality) > 0){
                    S104::SPI _spi = spis.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_spi);
                    }
                }
                break;
            case 3:
                if (_tokens.size() > 2){
                    if(_tokens[2] == "1"){
                        int _ivalue = atoi(_value.c_str());
                        _ivalue = _ivalue << 1 | 0x04;
                        _value = Converter::ToString(_ivalue);
                    }
                }
                if(dpis.at(_index).update(_value, _quality) > 0){
                    S104::DPI _dpi = dpis.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_dpi);
                    }
                }
                break;
            case 5:
                if(vtis.at(_index).update(_value, _quality) > 0){
                    S104::VTI _vti = vtis.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_vti);
                    }
                }
                break;
            case 9:
                if(this->nvas.at(_index).update(_value, _quality) > 0){
                    S104::NVA _nva = nvas.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_nva);
                    }
                }
                break;
            case 11:
                if(svas.at(_index).update(_value, _quality) > 0){
                    S104::SVA _sva = svas.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_sva);
                    }
                }
                break;
            case 13:
                if(this->fps.at(_index).update(_value, _quality) > 0){
                    S104::FP _fp = this->fps.at(_index);
                    for(int i = 0; i < MaxClient; i++){
                        sessions[i].update(_fp);
                    }
                }
                break;
        }
    }
}

void server::setDebug(string _debug){
    int __debug = atoi(_debug.c_str());
    switch(__debug){
        case 0:
            for (size_t i = 0; i < debug.size(); i++){
                debug.at(i) = false;
            }
            break;
        case 1:
            debug.at(0) = true;
            break;
        case 2:
            debug.at(1) = true;
            break;
        case 3:
            debug.at(2) = true;
            break;
        case 4:
            for (size_t i = 0; i < debug.size(); i++){
                debug.at(i) = true;
            }
            break;
    }
}

void server::addSPI(string _ref, int _address){
    S104::SPI spi(_address);
    spis.push_back(spi);
    int sz = spis.size();
    string _value = Converter::ToString(1) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >= 2) && spiSQ){
        if((spis.at(sz - 2).address + 1) != (spis.at(sz - 1).address)) spiSQ = oSQ;
    }
}

void server::addDPI(string _ref, int _address){
    S104::DPI dpi(_address);
    dpis.push_back(dpi);
    int sz = dpis.size();
    string _value = Converter::ToString(3) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >= 2) && dpiSQ){
        if((dpis.at(sz - 2).address + 1) != (dpis.at(sz - 1).address)) dpiSQ = oSQ;
    }
}

void server::addDPI(string _ref1, string _ref2, int _address){
    S104::DPI dpi(_address, true);
    dpis.push_back(dpi);
    int sz = dpis.size();
    string _value1 = Converter::ToString(3) + '.' + Converter::ToString(sz - 1) + ".0";
    string _value2 = Converter::ToString(3) + '.' + Converter::ToString(sz - 1) + ".1";
    inf.insert(pair<string, string>(_ref1, _value1));
    inf.insert(pair<string, string>(_ref2, _value2));
    if((sz >= 2) && dpiSQ){
        if((dpis.at(sz - 2).address + 1) != (dpis.at(sz - 1).address)) dpiSQ = oSQ;
    }
 }

void server::addVTI(string _ref, int _address){
    S104::VTI vti(_address);
    vtis.push_back(vti);
    int sz = vtis.size();
    string _value = Converter::ToString(5) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >= 2) && vtiSQ){
        if((vtis.at(sz - 2).address + 1) != (vtis.at(sz - 1).address)) vtiSQ = oSQ;
    }
}

void server::addNVA(string _ref, int _address){
    S104::NVA nva(_address);
    nvas.push_back(nva);
    int sz = nvas.size();
    string _value = Converter::ToString(9) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >= 2) && nvaSQ){
        if((nvas.at(sz - 2).address + 1) != (nvas.at(sz - 1).address)) nvaSQ = oSQ;
    }
}

void server::addSVA(string _ref, int _address){
    S104::SVA sva(_address);
    svas.push_back(sva);
    int sz = svas.size();
    string _value = Converter::ToString(11) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >= 2) && svaSQ){
        if((svas.at(sz - 2).address + 1) != (svas.at(sz - 1).address)) svaSQ = oSQ;
    }
}

void server::addFP(string _ref, int _address){
    S104::FP fp(_address);
    fps.push_back(fp);
    int sz = fps.size();
    string _value = Converter::ToString(13) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >=2) && fpSQ){
        if((fps.at(sz - 2).address + 1) != (fps.at(sz -1).address)) fpSQ = oSQ;
    }
}

void server::addFP(string _ref, int _address, float _a, float _b){
    S104::FP fp(_address, _a, _b);
    fps.push_back(fp);
    int sz = fps.size();
    string _value = Converter::ToString(13) + '.' + Converter::ToString(sz - 1);
    inf.insert(pair<string, string>(_ref, _value));
    if((sz >=2) && fpSQ){
        if((fps.at(sz - 2).address + 1) != (fps.at(sz -1).address)) fpSQ = oSQ;
    }
}

void server::addSCO(string _ref, int _address, string _sbo){
    S104::SCO sco(_ref, _address, _sbo);
    control.push_back(sco);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addDCO(string _ref, int _address, string _sbo){
    S104::DCO dco(_ref, _address, _sbo);
    control.push_back(dco);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addDCO(string _ref1, string _ref2, int _address, string _sbo){
    S104::DCO dco(_ref1, _ref2, _address, _sbo);
    control.push_back(dco);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addRCO(string _ref, int _address, string _sbo){
    S104::RCO rco(_ref, _address, _sbo);
    control.push_back(rco);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addRCO(string _ref1, string _ref2, int _address, string _sbo){
    S104::RCO rco(_ref1, _ref2, _address, _sbo);
    control.push_back(rco);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addNVO(string _ref, int _address, string _sbo){
    S104::NVO nvo(_ref, _address, _sbo);
    control.push_back(nvo);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addSVO(string _ref, int _address, string _sbo){
    S104::SVO svo(_ref, _address, _sbo);
    control.push_back(svo);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

void server::addFPO(string _ref, int _address, string _sbo){
    S104::FPO fpo(_ref, _address, _sbo);
    control.push_back(fpo);
    int sz = control.size();
    ctl.insert(pair<int, int>(_address, sz - 1));
}

}
