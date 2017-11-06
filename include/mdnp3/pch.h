#ifndef PCH_H_INCLUDED
#define PCH_H_INCLUDED

#define LINUX_ONLY

// #include your rarely changing headers here

#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <stdexcept>
#include <vector>
#include <list>
#include <queue>
#include <map>
//#include <memory>

typedef unsigned char byte;
typedef std::vector<byte> ByteVector;
typedef std::list<byte> ByteList;

#define MAXFRAME 292        // datalink data format
#define MAXSEGMENT 250      // transport data format
#define MAXFRAGMENT 2048    // application data format



#endif // PCH_H_INCLUDED
