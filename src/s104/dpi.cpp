#include <iostream>
#include <string>

#include "s104/s104.h"

using namespace std;

namespace S104
{

DPI::DPI()
{
    this->typeID = DOUBLE_POINT_INFORMATION;
    this->value[0] = 0x00;
    this->quality = 0x00;
}

DPI::DPI(int _address)
{
    this->address = _address;
    this->typeID = DOUBLE_POINT_INFORMATION;
    this->value[0] = 0x00;
    this->quality = 0x00;
}

DPI::DPI(int _address, bool _mk)
{
    this->address = _address;
    this->typeID = DOUBLE_POINT_INFORMATION;
    this->quality = 0x00;
    if(_mk) this->value[0] = 0x08;
    else this->value[0] = 0x00;
}

DPI::DPI(const DPI& obj)
{
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->value[0] = obj.value[0];
    this->quality = obj.quality;
    this->time = obj.time;
}

DPI::~DPI()
{

}

DPI& DPI::operator=(const DPI& obj){
    if (this != &obj){
            this->address = obj.address;
            this->typeID = obj.typeID;
            this->value[0] = obj.value[0];
            this->quality = obj.quality;
            this->time = obj.time;
    }
    return *this;
}

}
