#include <omega/PythonInterpreterWrapper.h>

#include "NumpyLoader.h"
#include  "signac.h"

///////////////////////////////////////////////////////////////////////////////
NumpyLoader::NumpyLoader()
{
    // Initialize the Numpy C API
    import_array();
}

///////////////////////////////////////////////////////////////////////////////
NumpyLoader::~NumpyLoader()
{
    typedef KeyValue<String, PyArrayObject*> Item;
    foreach(Item i, myObjects)
    {
        Py_DECREF(i.getValue());
    }
}

///////////////////////////////////////////////////////////////////////////////
size_t NumpyLoader::getNumRecords(Dataset* d)
{
    return myNumRecords;
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::open(const String& source)
{
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::addDimension(const String& name, PyObject* dataobject)
{
    if(PyArray_Check(dataobject)) 
    {
        PyArrayObject* ao = (PyArrayObject*)dataobject;
        Py_INCREF(ao);
    
        myObjects[name] = ao;
    
        npy_intp* shape = PyArray_SHAPE(ao);
        myNumRecords = shape[0];
        ofmsg("[NumpyLoader::addDimension] numRecords=<%1%>", %myNumRecords);
    } 
    else 
    {
        ofwarn("[NumpyLoader::addDimension] data object is not an array for dimension <%1%>", %name);
    }
}

///////////////////////////////////////////////////////////////////////////////
void NumpyLoader::load(Field* f)
{
    Dimension* dim = f->getDimension();
    if(myObjects.find(dim->id) != myObjects.end())
    {
        int j = dim->index;
        PyArrayObject* ao = myObjects[dim->id];
        int nd = PyArray_NDIM(ao);
        npy_intp* shape = PyArray_SHAPE(ao);
        PyArray_Descr* at = PyArray_DTYPE(ao);
        oassert(at.kind == 'f');
    
        size_t sstart = f->domain.start;
        size_t slen = f->domain.length;
        int sstride = f->domain.decimation;
        
        f->lock.lock();
                
        // Allocate field data
        float* fielddata = (float*)malloc((slen / sstride) * sizeof(float));
        
        size_t c = 0;
        for(size_t i = sstart; i < sstart + slen; i += sstride)
        {
            double* v = (double*)PyArray_GETPTR2(ao, i, j);
            fielddata[c++] = (float)*v;
        } 

        // Update field length
        f->data = (char*)fielddata;
        f->loaded = true;
        f->stamp = otimestamp();
        f->lock.unlock();
        
        ofmsg("Loaded field %1% %2% %3%", %dim->id %sstart %slen);

        Signac::instance->signalFieldLoaded(f);
    }
    else
    {
        ofwarn("[NumpyLoader::load] could not find dimension <%1%>", %dim->id);
    }
}
