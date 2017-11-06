#ifndef FRAMEDATAEVENTARG_H_INCLUDED
#define FRAMEDATAEVENTARG_H_INCLUDED

#include "mdnp3/EventArg.h"
#include "mdnp3/EventHandler.h"

namespace Dnp3Master
{

/*
Day la dinh dang lop du lieu datalink layer
*/
class FrameDataEventArg : public Dnp3Master::EventArg
{
public:
    FrameDataEventArg() : frame(NULL), length(0) {}
    virtual ~FrameDataEventArg() {}

    FrameDataEventArg(const FrameDataEventArg& obj)
    {
        for (int i = 0; i < obj.length; i++)
        {
            frame[i] = obj.frame[i];
        }
        length = obj.length;
    }

    FrameDataEventArg& operator=(const FrameDataEventArg& obj)
    {
        if (this != &obj)
        {
            for (int i = 0; i < obj.length; i++)
            {
                frame[i] = obj.frame[i];
            }
            length = obj.length;
        }
        return *this;
    }

    byte *frame;
    int length;
};

}

typedef EventHandlerBase<void, Dnp3Master::FrameDataEventArg> Dnp3FrameDataEvent;

#endif // FRAMEDATAEVENTARG_H_INCLUDED
