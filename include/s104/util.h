#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <iostream>
#include <cstdlib>
#include <stdlib.h>
#include <fstream>
#include <cstring>
#include <sstream>
#include <vector>

#include "server.h"

namespace S104{

void splitString(vector<string>& tokens, string& str, char delim);

server* initServer(string& path);

}
#endif // UTIL_H_INCLUDED
