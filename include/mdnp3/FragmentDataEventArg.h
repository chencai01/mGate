#ifndef FRAGMENTDATAEVENTARG_H_INCLUDED
#define FRAGMENTDATAEVENTARG_H_INCLUDED

#include "mdnp3/EventArg.h"
#include "mdnp3/EventHandler.h"

namespace Dnp3Master
{

/*
Day la dinh dang lop du lieu ma transport layer se gui len cho application layer
*/
class FragmentDataEventArg : public EventArg
{
public:
    FragmentDataEventArg() : fragment(NULL), length(0) {};
    virtual ~FragmentDataEventArg() {};

    FragmentDataEventArg(const FragmentDataEventArg& obj)
    {
        (*fragment).clear();
        ByteVector::size_type i;
        for (i = 0; i < obj.length; i++)
        {
            (*fragment).push_back((*obj.fragment)[i]);
        }
        length = fragment->size();
    }

    FragmentDataEventArg& operator=(const FragmentDataEventArg& obj)
    {
        if (this != &obj)
        {
            ByteVector::size_type i;
            for (i = 0; i < obj.length; i++)
            {
                (*fragment).push_back((*obj.fragment)[i]);
            }
            length = fragment->size();
        }
        return *this;
    }

    ByteVector* fragment;
    ByteVector::size_type length;
};
}

typedef EventHandlerBase<void, Dnp3Master::FragmentDataEventArg> Dnp3FragmentDataEvent;

#endif // FRAGMENTDATAEVENTARG_H_INCLUDED
