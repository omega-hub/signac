#ifndef __DATASET_H__
#define __DATASET_H__
#include <omega.h>

using namespace omega;

class Dataset;

///////////////////////////////////////////////////////////////////////////
//! A template for accessing gpu resources on multiple contexts.
template<typename T> class GpuRef
{
public:
    GpuRef()
    {
        memset(myStamps, 0, sizeof(myStamps));
    }
    Ref<T>& operator()(const GpuContext& context)
    {
        return myObjects[context.getId()];
    }
    Ref<T>& operator()(const DrawContext& context)
    {
        return myObjects[context.gpuContext->getId()];
    }
    double& stamp(const GpuContext& context)
    {
        return myStamps[context.getId()];
    }
    double& stamp(const DrawContext& context)
    {
        return myStamps[context.gpuContext->getId()];
    }
private:
    Ref<T> myObjects[GpuContext::MaxContexts];
    double myStamps[GpuContext::MaxContexts];
};

///////////////////////////////////////////////////////////////////////////////
struct Domain 
{
    Domain(size_t s = 0, size_t l = 0, int d = 0) : start(s), length(l), decimation(d) {}
    bool operator==(const Domain& rhs)
    {
        return start == rhs.start && 
            length == rhs.length && 
            decimation == rhs.decimation;
    }

    size_t start;
    size_t length;
    int decimation;
};

///////////////////////////////////////////////////////////////////////////////
class Dimension : public ReferenceType
{
public:
    Dimension();

    enum Type 
    {
        Float
    };

    Dataset* dataset;
    String id;
    String label;
    Type type;
    unsigned int index;

    double floatRangeMin;
    double floatRangeMax;

    size_t getElementSize();

};

class Dataset;

///////////////////////////////////////////////////////////////////////////////
class Field : public ReferenceType
{
public:
    Field(Dimension* info, const Domain& dom);

    char* data;
    bool loaded;
    bool loading;
    Domain domain;
    double stamp;

    Lock lock;

    Dimension* getDimension() { return myInfo; }
    GpuBuffer* getGpuBuffer(const DrawContext& dc);

    Vector2f range()
    {
        return Vector2f(myInfo->floatRangeMin, myInfo->floatRangeMax);
    }

    String getName()
    {
        return ostr("%1%<%2%,%3%,%4%>", %myInfo->id %domain.start %domain.length %domain.decimation);
    }

    size_t numElements()
    {
        return domain.length / domain.decimation;
    }

private:
    Ref<Dimension> myInfo;

    GpuRef<GpuBuffer> myGpuBuffer;
};

///////////////////////////////////////////////////////////////////////////////
class Filter : public WorkerTask
{
public:
    static const int MaxFields = 4;

    Filter();
    ~Filter();
    void setField(uint index, Field* f);
    void setRange(uint index, float fmin, float fmax);
    void setNormalizedRange(uint index, float fmin, float fmax);
    void execute(WorkerTask::TaskInfo* ti);
    void update();
    GpuBuffer* getIndexBuffer(const DrawContext& dc);
    uint getFilteredLength() { return myIndexLen; }

    double getIndexStamp() { return myIndexStamp; }

private:
    template<typename T> void filterKernel(double timestamp);


private:
    WorkerPool myUpdater;
    GpuRef<GpuBuffer> myGpuBuffer;
    Lock myLock;
    uint* myIndices;
    uint myIndexLen;
    Field* myField[MaxFields];
    float myMin[MaxFields];
    float myMax[MaxFields];
    int myNumFields;
    double myRangeStamp;
    double myIndexStamp;
};

class Loader;

///////////////////////////////////////////////////////////////////////////////
class Dataset : public ReferenceType
{
public:
    static const int MaxFields = 128;
    static void setDoublePrecision(bool enabled) { mysDoublePrecision = enabled; }
    static bool useDoublePrecision() { return mysDoublePrecision; }
public:
    // Creation function for the python API
    static Dataset* create(const String& name) { return new Dataset(name); }

    Dataset(const String& name);

    const String& getName() { return myName; }

    Dimension* addDimension(const String& name, Dimension::Type type);
    Field* addField(Dimension* dimension, const Domain& domain);
    Field* findField(Dimension* dimension, const Domain& domain);
    Field* getOrCreateField(Dimension* dimension, const Domain& domain);

    void setLoader(Loader* loader);
    Loader* getLoader() { return myLoader; }
    size_t getNumRecords();

    void load(Field* f);

private:
    static bool mysDoublePrecision;

    typedef List< Ref<Field> > FieldList;
    typedef List< Ref<Dimension> > DimensionList;

    DimensionList myDimensions;
    FieldList myFields;
    String myFilename;
    Loader* myLoader;
    String myName;
};
#endif