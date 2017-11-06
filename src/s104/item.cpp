#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#include "s104/s104.h"
#include "s104/bitconverter.h"

namespace S104
{

infItem::infItem()
{

}

infItem::~infItem()
{

}

int infItem::update(string _value, char _quality)
{
     int _ret = 0;
    char __value[4];
    switch(typeID)
    {
        case SINGLE_POINT_INFORMATION:
            {
                BitConverter::IntToBytes(__value, atoi(_value.c_str()));
                if((value[0] != __value[0]) || (quality != _quality))
                {
                    value[0] = __value[0] & 0x01;
                    quality = _quality & 0xF0;
                    gettimeofday(&time, NULL);
                    _ret = 1;
                }
            }
            break;
        case DOUBLE_POINT_INFORMATION:
            {
                BitConverter::IntToBytes(__value, atoi(_value.c_str()));
                if(value[0] >> 3){
                    // make double
                    char _v;
                    if(__value[0] >> 2) _v = (char)((__value[0] & 0x02) | (value[0] & 0x09));
                    else _v = (char)((_value[0]&0x01) | (value[0] & 0x0A));
                    if((value[0] != _v) || (quality != _quality)){
                        value[0] = _v;
                        quality = _quality;
                        gettimeofday(&time, NULL);
                        _ret = 1;
                    }
                }
                else{
                    if((value[0] != __value[0]) || (quality != _quality))
                    {
                        value[0] = __value[0] & 0x03;
                        quality = _quality & 0xF0;
                        gettimeofday(&time, NULL);
                        _ret = 1;
                    }
                }
            }
            break;
        case STEP_POSITION_INFORMATION:
            {
                BitConverter::IntToBytes(__value, atoi(_value.c_str()));
                if((value[0] != __value[0]) || (quality != _quality))
                {
                    value[0] = __value[0] & 0x7F;
                    quality = _quality & 0xF1;
                    gettimeofday(&time, NULL);
                    _ret = 1;
                }
            }
            break;
        case NORMALIZED_VALUE:
            {
                BitConverter::IntToBytes(__value, atoi(_value.c_str()));
                value[0] = __value[0];
                value[1] = __value[1];
                quality = _quality & 0xF1;
                _ret = 1;
            }
            break;
        case SCALED_VALUE:
            {
                BitConverter::IntToBytes(__value, atoi(_value.c_str()));
                value[0] = __value[0];
                value[1] = __value[1];
                quality = _quality & 0xF1;
                _ret = 1;
            }
            break;
        case SHORT_FLOATING_POINT_NUMBER:
            {
                float _temp = a * (float)atof(_value.c_str()) + b;
                BitConverter::FloatToBytes(__value, _temp);
                value[0] = __value[0];
                value[1] = __value[1];
                value[2] = __value[2];
                value[3] = __value[3];
                quality = _quality & 0xF1;
                _ret = 1;
            }
            break;
        default:
            break;
    }
    return _ret;
}

int infItem::update(string _value, char _quality, timeval _time)
{
    return 0;
}

ctlItem::ctlItem()
{

}

ctlItem::~ctlItem()
{

}

ctlItem::ctlItem(const ctlItem& obj)
{
    this->ref1 = obj.ref1;
    this->ref2 = obj.ref2;
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->value[0] = obj.value[0];
    this->value[1] = obj.value[1];
    this->value[2] = obj.value[2];
    this->value[3] = obj.value[3];
    this->sbo = obj.sbo;
}

ctlItem& ctlItem::operator=(const ctlItem& obj){
    if (this != &obj){
        this->ref1 = obj.ref1;
        this->ref2 = obj.ref2;
        this->address = obj.address;
        this->typeID = obj.typeID;
        this->value[0] = obj.value[0];
        this->value[1] = obj.value[1];
        this->value[2] = obj.value[2];
        this->value[3] = obj.value[3];
        this->sbo = obj.sbo;
    }
    return *this;
}

}
