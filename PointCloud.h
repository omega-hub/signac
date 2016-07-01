#ifndef __POINT_CLOUD__
#define __POINT_CLOUD__

#include <omega.h>

#include "Dataset.h"
#include "PointCloud.h"
#include "Program.h"

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
    Ref<Field> data[ProgramParams::MaxDataDimensions];

    PointBatch* batch;
    LOD* LOD;
    String myFilename;
    size_t batchStart;
    size_t batchLength;

    GpuRef<GpuDrawCall> drawCall;
    GpuRef<GpuArray> va;
};

///////////////////////////////////////////////////////////////////////////////
class PointBatch : public ReferenceType
{
public:
    PointBatch(PointCloud* owner);

    void addDrawable(LOD* lod, size_t start, size_t length);

    const AlignedBox3& getBoundingBox() { return myBBox; }
    void refreshFields();
    void draw(const DrawContext& c);

private:
    PointCloud* myOwner;
    List< Ref<BatchDrawable> > myDrawables;
    AlignedBox3 myBBox;
};

///////////////////////////////////////////////////////////////////////////////
class PointCloud : public NodeComponent
{
public:
    typedef List< Ref<PointCloud> > List;
    static const int MaxDataDimensions = 4;

public:
    PointCloud() : NodeComponent()
    {
        float maxf = numeric_limits<float>::max();
        float minf = -numeric_limits<float>::max();
        //myBBox.setExtents(Vector3f(minf, minf, minf), Vector3f(maxf, maxf, maxf));
        myMinDataBounds = Vector4f(maxf, maxf, maxf, maxf);
        myMaxDataBounds = Vector4f(minf, minf, minf, minf);
    }

    bool setOptions(const String& options);
    bool setDimensions(Dimension* x, Dimension* y, Dimension* z);
    void setData(int index, Dimension* fi) { myData[index] = fi; refreshFields();  }
    Dimension* getX() { return myX; }
    Dimension* getY() { return myY; }
    Dimension* getZ() { return myZ; }
    Dimension* getData(int index) { return myData[index]; }

    Dataset* getDataset() { return myDataset; }
    virtual void update(const UpdateContext& ctx);
    virtual const AlignedBox3* getBoundingBox() { return &myBBox; }
    virtual bool hasBoundingBox() { return true; }
    virtual void onAttached(SceneNode*);
    virtual void onDetached(SceneNode*);

    void draw(const DrawContext& c);

    void setProgram(Program* p);
    Program* getProgram() { return myProgram; }

    void setMappingProgram(Program* p);
    Program* getMappingProgram() { return myMappingProgram; }


private:
    void refreshFields();

private:
    Ref<Dataset> myDataset;
    Ref<Dimension> myX;
    Ref<Dimension> myY;
    Ref<Dimension> myZ;
    Ref<Dimension> myData[MaxDataDimensions];
    Ref<Program> myProgram;
    
    size_t myPointsPerBatch;

    Vector<LOD> myLODLevels;

    ::List< Ref<PointBatch> > myBatches;
    AlignedBox3 myBBox;
    Vector4f myMinDataBounds;
    Vector4f myMaxDataBounds;

    // Image render
    GpuRef<RenderTarget> myRenderTarget;
    GpuRef<Texture> myRenderOutput;
    GpuRef<GpuArray> myFullscreenQuad;
    GpuRef<GpuDrawCall> myFullscreenDraw;
    Ref<Program> myMappingProgram;
};

#endif