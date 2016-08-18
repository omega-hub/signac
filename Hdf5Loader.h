#ifndef __HDF5LOADER_H__
#define __HDF5LOADER_H__

#include "Loader.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class Hdf5Loader : public Loader
{
public:
    ~Hdf5Loader();

    void open(const String& source);
    void load(Field* f);
    size_t getNumRecords(Dataset* d);

protected:
    String myFilename;
};
#endif
