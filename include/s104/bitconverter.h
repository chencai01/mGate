#ifndef BITCONVERTER_H_INCLUDED
#define BITCONVERTER_H_INCLUDED

#include <sys/time.h>
#include <ctime>

namespace BitConverter{

void ShortToBytes(char arr[2], short Value);

void IntToBytes(char arr[4], int Value);

void FloatToBytes(char arr[4], float Value);

void TimeToBytes(char arr[7], timeval& Time);

}

#endif // BITCONVERTER_H_INCLUDED
