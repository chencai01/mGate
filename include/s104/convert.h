#ifndef CONVERT_H_INCLUDED
#define CONVERT_H_INCLUDED

#include <iostream>
#include <sstream>
#include <stdio.h>

using namespace std;

namespace Converter{

float ToFloat(char arr[]);

string ToString(int value);

string ToString(float value);

string ToString(timeval time);

}

#endif // CONVERT_H_INCLUDED
