#ifndef __LOADER_H__
#define __LOADER_H__
#include <omega.h>
#include "Dataset.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class Loader : public ReferenceType
{
public:
    virtual void open(const String& source) = 0;
    virtual void load(Field* f) = 0;
};
#endif