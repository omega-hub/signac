#include <omega/PythonInterpreterWrapper.h>

#include "NumpyLoader.h"

///////////////////////////////////////////////////////////////////////////////
NumpyLoader::NumpyLoader()
{
    // Initialize the Numpy C API
    import_array();
}

///////////////////////////////////////////////////////////////////////////////
NumpyLoader::~NumpyLoader()
{
}

///////////////////////////////////////////////////////////////////////////////
size_t NumpyLoader::getNumRecords(Dataset* d)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::open(const String& source)
{
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::addDimension(const String& name, PyObject* dataobject)
{
    // NOTE: unsafe cast here. we should do some basic sanity checks on dataobject
    // to make sure it's a numpy array.
    myObject = (PyArrayObject*)dataobject;
    
    int nd = PyArray_NDIM(myObject);
    //npy_intp* shape = PyArray_SHAPE(myObject);
    int size = PyArray_SIZE(myObject);
    //ofmsg("array size: %1%", %size);
    
    int* data = (int*)PyArray_DATA(myObject);
    for(int i = 0; i < size; i++) printf("%d ", data[i]);
    
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::load(Field* f)
{

}
