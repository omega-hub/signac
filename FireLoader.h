#ifndef __FIRELOADER_H__
#define __FIRELOADER_H__

#include "Hdf5Loader.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class FireLoader : public Hdf5Loader
{
public:
    size_t getNumRecords(Dataset* d);
};
#endif
