#include <iostream>
#include <string>

#include "s104/s104.h"

using namespace std;

namespace S104
{

VTI::VTI()
{
    this->typeID = STEP_POSITION_INFORMATION;
    this->value[0] = 0;
    this->quality = 0x00;
}

VTI::VTI(int _address)
{
    this->address = _address;
    this->typeID = STEP_POSITION_INFORMATION;
    this->value[0] = 0;
    this->quality = 0x00;
}

VTI::VTI(const VTI& obj)
{
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->value[0] = obj.value[0];
    this->quality = obj.quality;
    this->time = obj.time;
}

VTI::~VTI()
{

}

VTI& VTI::operator=(const VTI& obj){
    if (this != &obj){
            address = obj.address;
            typeID = obj.typeID;
            value[0] = obj.value[0];
            quality = obj.quality;
            time = obj.time;
    }
    return *this;
}

}
