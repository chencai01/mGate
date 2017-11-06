#include <iostream>
#include <string>

#include "s104/s104.h"

namespace S104
{

SPI::SPI()
{
    this->typeID = SINGLE_POINT_INFORMATION;
    this->value[0] = 0x00;
    this->quality = 0x00;
}

SPI::SPI(int _address)
{
    this->address = _address;
    this->typeID = SINGLE_POINT_INFORMATION;
    this->value[0] = 0x00;
    this->quality = 0x00;
}

SPI::SPI(const SPI& obj)
{
    this->address = obj.address;
    this->typeID = obj.typeID;
    this->value[0] = obj.value[0];
    this->quality = obj.quality;
    this->time = obj.time;
}

SPI::~SPI()
{

}

SPI& SPI::operator=(const SPI& obj){
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
