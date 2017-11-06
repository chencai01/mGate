#include <iostream>
#include <string>

#include "s104/s104.h"

namespace S104
{

SCO::SCO()
{
    this->typeID = 0x2D;
    this->value[0] = 0;
    this->sbo = false;
}

SCO::SCO(string _ref, int _address)
{
    this->ref1 = _ref;
    this->address = _address;
    this->typeID = 0x2D;
    this->value[0] = 0;
    this->sbo = false;
}

SCO::SCO(string _ref, int _address, string _sbo)
{
    this->ref1 = _ref;
    this->address = _address;
    this->typeID = 0x2D;
    this->value[0] = 0;
    if(_sbo == "sop") this->sbo = true;
    else this->sbo = false;
}

SCO::~SCO()
{

}

}
