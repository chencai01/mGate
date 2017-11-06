#ifndef SEGMENTDATAEVENTARG_H_INCLUDED
#define SEGMENTDATAEVENTARG_H_INCLUDED

#include "mdnp3/EventArg.h"
#include "mdnp3/EventHandler.h"

namespace Dnp3Master
{

/*
Day la dinh dang lop du lieu datalink layer se gui len transport layer
*/
class SegmentDataEventArg : public Dnp3Master::EventArg
{
public:
    SegmentDataEventArg() : segment(NULL), length(0) {}
    virtual ~SegmentDataEventArg() {}

    SegmentDataEventArg(const SegmentDataEventArg& obj)
    {
        for (int i = 0; i < obj.length; i++)
        {
            segment[i] = obj.segment[i];
        }
        length = obj.length;
    }

    SegmentDataEventArg& operator=(SegmentDataEventArg& obj)
    {
        if (this != &obj)
        {
            for (int i = 0; i < obj.length; i++)
            {
                segment[i] = obj.segment[i];
            }
            length = obj.length;
        }
        return *this;
    }

    byte *segment;
    int length;
};

} // Dnp3Master namespace

typedef EventHandlerBase<void, Dnp3Master::SegmentDataEventArg> Dnp3SegmentDataEvent;

#endif // SEGMENTDATAEVENTARG_H_INCLUDED




