#ifndef __FILTER_H__
#define __FILTER_H__

#include <omega.h>

using namespace omega;

class Dataset;
class Field;

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

#endif