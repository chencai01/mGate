#include "s104/convert.h"

namespace Converter{

float ToFloat(char arr[]){
    char a[4];
    a[0] = arr[0]; // 0x00
    a[1] = arr[1]; // 0x00
    a[2] = arr[2]; // 0x00
    a[3] = arr[3]; // 0x40
    return *(float*)(&a);
}

string ToString(int value){
    string _s;
    stringstream _ss;
    _ss << value;
    _s = _ss.str();
    return _s;
}

string ToString(float value){
    string _s;
    stringstream _ss;
    _ss << value;
    _s = _ss.str();
    return _s;
}

string ToString(timeval time){
    char _c[19];
    tm *_time = localtime(&time.tv_sec);
    sprintf(_c, "%04d-%02d-%02d %02d:%02d:%02d", _time->tm_year + 1900, _time->tm_mon + 1, _time->tm_mday, _time->tm_hour, _time->tm_min, _time->tm_sec);
    string _s(_c);
    return _s;
}

}
