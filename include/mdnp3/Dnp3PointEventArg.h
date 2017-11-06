#ifndef DNP3POINTEVENTARG_H_INCLUDED
#define DNP3POINTEVENTARG_H_INCLUDED

#include "mdnp3/EventArg.h"
#include "mdnp3/Dnp3Points.h"
#include "mdnp3/EventHandler.h"


namespace Dnp3Master
{
/*
Day la dinh dang du lieu dnp3
*/
class Dnp3PointEventArg : public EventArg
{
public:
    Dnp3PointEventArg() : points(NULL), length(0) {}
    virtual ~Dnp3PointEventArg() {}

    std::vector<Dnp3Point>* points;
    std::vector<Dnp3Point>::size_type length;
};
}

typedef EventHandlerBase<void, Dnp3Master::Dnp3PointEventArg> Dnp3PointDataEvent;


#endif // DNP3POINTEVENTARG_H_INCLUDED

