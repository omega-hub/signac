#ifndef __NUMPYLOADER_H__
#define __NUMPYLOADER_H__

#include "Loader.h"

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include "numpy/arrayobject.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class NumpyLoader : public Loader
{
public:
    static const int MaxParts = 8;
public:
    NumpyLoader();
    ~NumpyLoader();
    size_t getNumRecords(Dataset* d);
    void open(const String& source);
    void load(Field* f);
    void addDimension(const String& name, PyObject* dataobject);

private:
    Dictionary<String, PyArrayObject*> myObjects;
    size_t myNumRecords;
};
#endif
