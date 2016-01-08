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

    union 
    {
        float floatRangeMin;
    };
    union
    {
        float floatRangeMax;
    };

    size_t getElementSize()
    {
        switch(type)
        {
        case Float: return 4;
        }
        return 0;
    }
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

public:
    Dataset();

    Field* addField(const String& id, FieldInfo::FieldType, String name);

    bool open(const String& filename, Loader* loader);

    void load(Field* f);

private:
    typedef List< Ref<Field> > FieldList;
    FieldList myFields;
    String myFilename;
    Loader* myLoader;
};
#endif