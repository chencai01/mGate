#include <iostream>
#include <string>

#include "s104/s104.h"

using namespace std;

namespace S104{

FP::FP()
{
    this->typeID = SHORT_FLOATING_POINT_NUMBER;
    this->a = 1.0;
    this->b = 0x0;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    this->quality = 0x00;
}

FP::FP(int _address)
{
    this->address = _address;
    this->typeID = SHORT_FLOATING_POINT_NUMBER;
    this->a = 1.0;
    this->b = 0.0;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    this->quality = 0x00;
}

FP::FP(int _address, float _a, float _b){
    this->address = _address;
    this->typeID = SHORT_FLOATING_POINT_NUMBER;
    this->a = _a;
    this->b = _b;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    this->quality = 0x00;
}

FP::FP(const FP& obj)
{
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->a = obj.a;
    this->b = obj.b;
    this->value[0] = obj.value[0];
    this->value[1] = obj.value[1];
    this->value[2] = obj.value[2];
    this->value[3] = obj.value[3];
    this->quality = obj.quality;
}

FP::~FP()
{

}

FP& FP::operator=(const FP& obj){
    if (this != &obj){
            address = obj.address;
            typeID = obj.typeID;
            a = obj.a;
            b = obj.b;
            value[0] = obj.value[0];
            value[1] = obj.value[1];
            value[2] = obj.value[2];
            value[3] = obj.value[3];
            quality = obj.quality;
    }
    return *this;
}

}
