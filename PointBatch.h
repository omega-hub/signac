#ifndef __POINT_BATCH__
#define __POINT_BATCH__

#include <omega.h>

#include "Dataset.h"

using namespace omega;

class PointCloud;
class PointBatch;

///////////////////////////////////////////////////////////////////////////////
class LOD
{
public:
    LOD(int dmin, int dmax, int dc) :
        distmin(dmin),
        distmax(dmax),
        dec(dc) {}
    int distmin;
    int distmax;
    int dec;

    static bool parse(const String& options, Vector<LOD>* lodlevels, size_t* pointsPerBatch);
};

///////////////////////////////////////////////////////////////////////////////
class BatchDrawable : public ReferenceType
{
public:
    BatchDrawable(PointBatch* batch, LOD* lod, size_t start, size_t length) :
        LOD(lod),
        batchStart(start),
        batchLength(length)
    {}

    Ref<Field> x;
    Ref<Field> y;
    Ref<Field> z;
    Ref<Field> data;
    Ref<Field> datax;
    Ref<Field> datay;
    Ref<Field> dataz;
    Ref<Field> size;
    Ref<Field> filter;

    PointBatch* batch;
    LOD* LOD;
    String myFilename;
    size_t batchStart;
    size_t batchLength;

    GpuRef<GpuDrawCall> drawCall;
    GpuRef<GpuArray> va;
    GpuRef<Texture> colormap;
};

///////////////////////////////////////////////////////////////////////////////
class PointBatch : public ReferenceType
{
public:
    PointBatch(PointCloud* owner);

    void addDrawable(LOD* lod, size_t start, size_t length);

    bool hasBoundingBox();
    const AlignedBox3& getBoundingBox();
    void refreshFields();
    void draw(const DrawContext& c);

private:
    PointCloud* myOwner;
    List< Ref<BatchDrawable> > myDrawables;
    AlignedBox3 myBBox;
};

#endif