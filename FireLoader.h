#ifndef __FIRELOADER_H__
#define __FIRELOADER_H__

#include "Hdf5Loader.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
//! Multipart HDF5 file loader
class FireLoader : public Loader
{
public:
    static const int MaxParts = 8;
public:
    FireLoader();
    ~FireLoader();
    size_t getNumRecords(Dataset* d);
    void open(const String& source);
    void load(Field* f);

private:
    Vector<String> mySnapshotPaths;
    size_t* myRecordsPerFile;
    Vector< Ref<Hdf5Loader> > myLoaders;
    Dictionary<Dataset*, size_t> myNumRecords;
};
#endif
