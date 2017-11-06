#include <iostream>
#include <string>

#include "s104/s104.h"

namespace S104
{

FPO::FPO()
{
    this->typeID = 0x32;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    this->sbo = false;
}

FPO::FPO(string _ref, int _address)
{
    this->ref1 = _ref;
    this->address = _address;
    this->typeID = 0x32;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    this->sbo = false;
}

FPO::FPO(string _ref, int _address, string _sbo)
{
    this->ref1 = _ref;
    this->address = _address;
    this->typeID = 0x32;
    this->value[0] = 0;
    this->value[1] = 0;
    this->value[2] = 0;
    this->value[3] = 0;
    if(_sbo == "sop") this->sbo = true;
    else this->sbo = false;
}

FPO::~FPO()
{

}

}
