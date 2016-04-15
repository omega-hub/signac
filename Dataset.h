#ifndef __DATASET_H__
#define __DATASET_H__
#include <omega.h>

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class FieldInfo : public ReferenceType
{
public:
    FieldInfo();

    enum FieldType 
    {
        Float
    };

    String id;
    String label;
    FieldType type;
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
    Field(FieldInfo* info, Dataset* ds);

    Dataset* dataset;
    char* data;
    bool loaded;
    bool loading;
    unsigned int length;
    double stamp;

    Lock lock;

    FieldInfo* getInfo() { return myInfo; }
    VertexBuffer* getGpuBuffer(const DrawContext& dc);

    Vector2f range()
    {
        return Vector2f(myInfo->floatRangeMin, myInfo->floatRangeMax);
    }

private:
    Ref<FieldInfo> myInfo;

    GpuRef<VertexBuffer> myGpuBuffer;
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
    VertexBuffer* getIndexBuffer(const DrawContext& dc);
    uint getFilteredLength() { return myIndexLen; }

    double getIndexStamp() { return myIndexStamp; }

private:
    template<typename T> void filterKernel(double timestamp);


private:
    WorkerPool myUpdater;
    GpuRef<VertexBuffer> myGpuBuffer;
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
    Dataset();

    Field* addField(const String& id, FieldInfo::FieldType, String name);

    bool open(const String& filename, Loader* loader);

    void load(Field* f);

private:
    static bool mysDoublePrecision;

    typedef List< Ref<Field> > FieldList;
    FieldList myFields;
    String myFilename;
    Loader* myLoader;
};
#endif