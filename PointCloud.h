#ifndef __POINT_CLOUD__
#define __POINT_CLOUD__

#include <omega.h>

#include "Dataset.h"
#include "PointBatch.h"
#include "Program.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class PointCloud : public NodeComponent
{
    friend class PointBatch;
public:
    typedef List< Ref<PointCloud> > List;
    static PointCloud* create(const String& name) { return new PointCloud(name); }
public:
    PointCloud(const String& name);

    bool setOptions(const String& options);
    bool setDimensions(Dimension* x, Dimension* y, Dimension* z);
    void setData(Dimension* fi);
    void setVectorData(Dimension* x, Dimension* y, Dimension* z);
    void setSize(Dimension* fi);
    void setFilter(Dimension* fi);
    Dimension* getX() { return myX; }
    Dimension* getY() { return myY; }
    Dimension* getZ() { return myZ; }
    Dimension* getDataX() { return myDataX; }
    Dimension* getDataY() { return myDataY; }
    Dimension* getDataZ() { return myDataZ; }
    Dimension* getData() { return myData; }
    Dimension* getSize() { return mySize; }
    Dimension* getFilter() { return myFilter; }

    void setPointScale(float scale);
    void normalizeFilterBounds(bool enabled);
    void setFilterBounds(float fmin, float fmax);
    void setDecimation(int dec);

    void setColormap(PixelData* colormap);
    PixelData* getColormap() { return myColormap; }
    void setColor(const Color& c);
    void setFocusPosition(const Vector3f d);

    Dataset* getDataset() { return myDataset; }
    virtual void update(const UpdateContext& ctx);
    virtual const AlignedBox3* getBoundingBox() { return &myBBox; }
    virtual bool hasBoundingBox() { return true; }

    void draw(const DrawContext& c);

    void setProgram(Program* p);
    Program* getProgram() { return myProgram; }

    bool isVisible() { return myVisible && getOwner() != NULL && getOwner()->isVisible(); }
    void setVisible(bool v) { myVisible = v; }

private:
    void refreshFields();

private:
    bool myVisible;
    Ref<Dataset> myDataset;
    Ref<Dimension> myX;
    Ref<Dimension> myY;
    Ref<Dimension> myZ;
    Ref<Dimension> myData;
    Ref<Dimension> myDataX;
    Ref<Dimension> myDataY;
    Ref<Dimension> myDataZ;
    Ref<Dimension> mySize;
    Ref<Dimension> myFilter;
    Ref<Program> myProgram;
    Ref<ProgramParams> myProgramParams;
    Ref<PixelData> myColormap;

    size_t myPointsPerBatch;

    Vector<LOD> myLODLevels;

    ::List< Ref<PointBatch> > myBatches;
    AlignedBox3 myBBox;
    Vector4f myMinDataBounds;
    Vector4f myMaxDataBounds;

    Ref<Stat> myBatchDrawStat;
};


#endif