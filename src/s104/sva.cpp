#include <iostream>
#include <string>

#include "s104/s104.h"

using namespace std;

namespace S104{

SVA::SVA()
{
    this->typeID = SCALED_VALUE;
    this->value[0] = 0;
    this->value[1] = 0;
    this->quality = 0x00;
}

SVA::SVA(int _address)
{
    this->address = _address;
    this->typeID = SCALED_VALUE;
    this->value[0] = 0;
    this->value[1] = 0;
    this->quality = 0x00;
}

SVA::SVA(const SVA& obj)
{
    this->address = obj.address;
    this->value[0] = obj.value[0];
    this->value[1] = obj.value[1];
    this->quality = obj.quality;
}

SVA::~SVA()
{

}

SVA& SVA::operator=(const SVA& obj){
    if (this != &obj){
            address = obj.address;
            typeID = obj.typeID;
            value[0] = obj.value[0];
            quality = obj.quality;
    }
    return *this;
}

}
