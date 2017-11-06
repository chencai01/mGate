#include <iostream>
#include <string>

#include "s104/s104.h"

using namespace std;

namespace S104{

NVA::NVA()
{
    this->typeID = NORMALIZED_VALUE;
    this->value[0] = 0;
    this->value[1] = 0;
    this->quality =0x00;
}

NVA::NVA(int _address)
{
    this->address = _address;
    this->typeID = NORMALIZED_VALUE;
    this->value[0] = 0;
    this->value[1] = 0;
    this->quality = 0x00;
}

NVA::NVA(const NVA& obj)
{
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->value[0] = obj.value[0];
    this->value[1] = obj.value[1];
    this->quality = obj.quality;
}

NVA::~NVA()
{

}

NVA& NVA::operator=(const NVA& obj){
    if (this != &obj){
            address = obj.address;
            typeID = obj.typeID;
            value[0] = obj.value[0];
            quality = obj.quality;
    }
    return *this;
}

}
