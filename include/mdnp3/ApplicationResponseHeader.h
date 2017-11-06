#ifndef APPLICATIONRESPONSEHEADER_H_INCLUDED
#define APPLICATIONRESPONSEHEADER_H_INCLUDED

#include "mdnp3/ApplicationRequestHeader.h"
#include "mdnp3/InternalIndication.h"

namespace Dnp3Master
{

class ApplicationResponseHeader
{
public:
    ApplicationResponseHeader() :
        AC(0x00), FC(APFunctionCode::CONFRIM), IIN()
    {}
    virtual ~ApplicationResponseHeader() {}

    ApplicationControl AC;
    APFunctionCode::APFunctionCode FC;
    InternalIndication IIN;
};

}

#endif // APPLICATIONRESPONSEHEADER_H_INCLUDED
