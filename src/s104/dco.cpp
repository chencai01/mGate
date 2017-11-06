#include <iostream>
#include <string>

#include "s104/s104.h"

namespace S104
{

DCO::DCO()
{
    this->typeID = 0x2E;
    this->value[0] = 0;
    this->sbo = false;
}

DCO::DCO(string _ref, int _address)
{
    this->ref1 = _ref;
    this->ref2 = "";
    this->address = _address;
    this->typeID = 0x2E;
    this->value[0] = 0;
    this->sbo = false;
}

DCO::DCO(string _ref1, string _ref2, int _address){
    this->ref1 = _ref1;
    this->ref2 = _ref2;
    this->address = _address;
    this->typeID = 0x2E;
    this->value[0] = 0;
    this->sbo = false;
}

DCO::DCO(string _ref, int _address, string _sbo)
{
    this->ref1 = _ref;
    this->ref2 = "";
    this->address = _address;
    this->typeID = 0x2E;
    this->value[0] = 0;
    if(_sbo == "sop") this->sbo = true;
    else this->sbo = false;
}

DCO::DCO(string _ref1, string _ref2, int _address, string _sbo){
    this->ref1 = _ref1;
    this->ref2 = _ref2;
    this->address = _address;
    this->typeID = 0x2E;
    this->value[0] = 0;
    if(_sbo == "sop") this->sbo = true;
    else this->sbo = false;
}

DCO::~DCO()
{

}

}
