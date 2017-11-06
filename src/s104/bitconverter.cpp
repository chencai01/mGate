#include "s104/bitconverter.h"


namespace BitConverter{

void ShortToBytes(char arr[2], short Value){
    arr[0] = Value & 0xFF;
    arr[1] = Value >> 8 & 0xFF;
}

void IntToBytes(char arr[4], int Value){
    arr[0] = Value & 0xFF;
    arr[1] = Value >> 8 & 0xFF;
    arr[2] = Value >> 16 & 0xFF;
    arr[3] = Value >> 24 & 0xFF;
}

void FloatToBytes(char arr[4], float Value){
    char * _value;
    _value = (char*)&Value;
    arr[0] = _value[0];
    arr[1] = _value[1];
    arr[2] = _value[2];
    arr[3] = _value[3];
}

void TimeToBytes(char arr[7], timeval& Time){
    tm *_time = localtime(&Time.tv_sec);
    int _milisecond = _time->tm_sec*1000 + Time.tv_usec/1000;
    char _ms[4];
    IntToBytes(_ms, _milisecond);
    arr[0] = _ms[0];
    arr[1] = _ms[1];
    arr[2] = (_time->tm_min & 0xFF);
    arr[3] = (_time->tm_hour & 0xFF);
    arr[4] = (_time->tm_mday & 0xFF);
    arr[5] = (_time->tm_mon + 1) & 0xFF;
    arr[6] = (_time->tm_year - 100) & 0xFF;
}

}
