#include "s104/util.h"

namespace S104{

void splitString(vector<string>& tokens, string& str, char delim)
{
    stringstream ss(str);
    string _token;
    while(getline(ss, _token, delim)){
        tokens.push_back(_token);
    }
}

server* initServer(string& path)
{
    server* s = new server();
    ifstream _inFile;
    _inFile.open(path.c_str());
    if(!_inFile.good()){
        cerr << "Error opening config file." << endl;
        return NULL;
    }
    else{
        string _line;
        while(getline(_inFile, _line)){
            char _c;
            vector<string> _tokens;
            //getline(_inFile, _line);
            if(_line == "[channel]"){
                while (getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        if(_tokens[0] == "MaxClient") s->MaxClient = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "asduAddress") s->asduAddress = (unsigned short)atoi(_tokens[1].c_str());
                        if(_tokens[0] == "T0") s->T0 = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "T1") s->T1 = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "T2") s->T2 = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "T3") s->T3 = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "K") s->K = (short)atoi(_tokens[1].c_str());
                        if(_tokens[0] == "W") s->W = (short)atoi(_tokens[1].c_str());
                        if(_tokens[0] == "MVPeriodicCycle") s->Periodic = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "BackgroundScanCycle") s->Background = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "SBOTimeOut") s->SBOTimeout = atoi(_tokens[1].c_str());
                        if(_tokens[0] == "client"){
                            vector<string> _ips;
                            splitString(_ips, _tokens[1], ';');
                            for(size_t i = 0; i < _ips.size(); i++){
                                s->IPs.insert(pair<string, int>(_ips[i], i));
                            }
                        }
                    }
                    _c = (char)(_inFile.peek());
                    if (_c == '[') break; // reached [SPIs]
                }
            }
            if(_line == "[SPIs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        s->addSPI(_ref, _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [DPIs]
                }
            }
            if(_line == "[DPIs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        vector<string> _refs;
                        splitString(_refs, _ref, ':');
                        if(_refs.size() > 1) s->addDPI(_refs[0], _refs[1], _address);
                        else s->addDPI(_refs[0], _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [VTIs]
                }
            }
            if(_line == "[VTIs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        s->addVTI(_ref, _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [NVAs]
                }
            }
            if(_line == "[NVAs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        s->addNVA(_ref, _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [SVAs]
                }
            }
            if(_line == "[SVAs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        s->addSVA(_ref, _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [FPs]
                }
            }
            if(_line == "[FPs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 2){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _coef = _tokens[2];
                        vector<string> _coefs;
                        splitString(_coefs, _coef, '|');
                        float _a = atof(_coefs[0].c_str());
                        float _b = atof(_coefs[1].c_str());
                        s->addFP(_ref, _address, _a, _b);
                    }
                    else if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        s->addFP(_ref, _address);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [SCOs]
                }
            }
            if(_line == "[SCOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        s->addSCO(_ref, _address, _sbo);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [DCOs]
                }
            }
            if(_line == "[DCOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        vector<string> _refs;
                        splitString(_refs, _ref, ':');
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        if(_refs.size() > 1) s->addDCO(_refs[0], _refs[1], _address, _sbo);
                        else s->addDCO(_refs[0], _address, _sbo);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [RCOs]
                }
            }
            if(_line == "[RCOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        vector<string> _refs;
                        splitString(_refs, _ref, ':');
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        if(_refs.size() > 1) s->addRCO(_refs[0], _refs[1], _address, _sbo);
                        else s->addRCO(_refs[0], _address, _sbo);
                    }
                    _c = (char)(_inFile.peek());
                    if(_c == '[') break; // reached [NVOs]
                }
            }
            if(_line == "[NVOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        s->addNVO(_ref, _address, _sbo);
                    }
                    _c = (char)(_inFile.peek()); //reached [SVOs]
                    if(_c == '[') break;
                }
            }
            if(_line == "[SVOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        s->addSVO(_ref, _address, _sbo);
                    }
                    _c = (char)(_inFile.peek()); //reached [FPOs]
                    if(_c == '[') break;
                }
            }
            if(_line == "[FPOs]"){
                while(getline(_inFile, _line)){
                    //getline(_inFile, _line);
                    _tokens.clear();
                    splitString(_tokens, _line, '=');
                    if(_tokens.size() > 1){
                        string _ref = _tokens[0];
                        unsigned int _address = atoi(_tokens[1].c_str());
                        string _sbo = _tokens[2];
                        s->addFPO(_ref, _address, _sbo);
                    }
                }
            }
        }
        return s;
    }
}

}
