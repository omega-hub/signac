#ifndef __BINARY_LOADER__
#define __BINARY_LOADER__

#include <omega.h>
#include "Loader.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class BinaryLoader : public Loader
{
public:
    BinaryLoader();
    virtual ~BinaryLoader();

    virtual void open(const String& source);
    virtual void load(Field* f);
    virtual size_t getNumRecords();
    virtual int getDimensions();
    virtual bool getBounds(const Domain& d, float* bounds);

    //virtual bool load(BatchDrawable* batch, const String& filename) = 0;
    //virtual bool getBounds(const String& filename, size_t readStart, size_t readLength, int decimation, float* bounds, int dimensions);

private:
    bool readBoundsFile(const String& filename, float* bounds);

    template<typename T>
    void readXYZ(
        const String& filename,
        size_t readStart, size_t readLength, int decimation,
        Vector<Vector3f>* points, Vector<Vector4f>* colors,
        size_t* numPoints,
        Vector3f* pointmin,
        Vector3f* pointmax,
        Vector4f* rgbamin,
        Vector4f* rgbamax) const;

private:
    String myFilename;
    size_t myNumRecords;
    WorkerPool myLoaderPool;
};

#endif