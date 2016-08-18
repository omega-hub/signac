#include "Dataset.h"
#include "Filter.h"

///////////////////////////////////////////////////////////////////////////////
Filter::Filter() :
myRangeStamp(0),
myIndexStamp(0),
myNumFields(0),
myIndices(NULL),
myIndexLen(-1)
{
    memset(myField, 0, sizeof(myField));
    myUpdater.start(1);
}

///////////////////////////////////////////////////////////////////////////////
Filter::~Filter()
{
    myUpdater.stop();
}


///////////////////////////////////////////////////////////////////////////////
void Filter::setField(uint index, Field* f)
{
    if(f != NULL) {
        myField[index] = f;
        myMin[index] = f->getDimension()->floatRangeMin;
        myMax[index] = f->getDimension()->floatRangeMax;

        // Count the number of consecutive valid fields.
        myNumFields = 0;
        foreach(Field* f, myField) if(f != NULL) myNumFields++; else break;
    }
}

///////////////////////////////////////////////////////////////////////////////
void Filter::setRange(uint index, float fmin, float fmax)
{
    myMin[index] = fmin;
    myMax[index] = fmax;
    myRangeStamp = otimestamp();
    myUpdater.clearQueue();
    myUpdater.queue(this);
}

///////////////////////////////////////////////////////////////////////////////
void Filter::setNormalizedRange(uint index, float fmin, float fmax)
{
    float fm = myField[index]->range()[0];
    float fM = myField[index]->range()[1];
    float delta = fM - fm;
    myMin[index] = fm + delta * fmin;
    myMax[index] = fm + delta * fmax;
    myRangeStamp = otimestamp();
    myUpdater.clearQueue();
    myUpdater.queue(this);
}

///////////////////////////////////////////////////////////////////////////////
template<typename T> 
void Filter::filterKernel(double timestamp)
{
    uint sz = static_cast<uint>(myField[0]->domain.length);
    uint* indices = (uint*)malloc(sz * sizeof(uint));
    memset(indices, 0, sz * sizeof(uint));

    uint len = 0;
    for(uint i = 0; i < sz; i++)
    {
        // if the filter stamp was updated, we are processing stale data. exit now.
        if(myRangeStamp > timestamp)
        {
            free(indices);
            return;
        }

        bool pass = true;
        for(uint j = 0; j < myNumFields; j++)
        {
            T* d = (T*)myField[j]->data;
            if(d[i] < myMin[j] || d[i] > myMax[j])
            {
                pass = false;
                break;
            }
        }

        if(pass)
        {
            indices[len] = i; len++;
        }
    }
    // Done filtering. copy the new indices over the old ones.
    myLock.lock();
    if(myIndices != NULL) free(myIndices);
    myIndices = indices;
    myIndexLen = len;
    //ofmsg("index generated - length = %1%", %myIndexLen);
    myIndexStamp = otimestamp();
    myLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////
void Filter::execute(WorkerTask::TaskInfo* ti)
{
    if(myNumFields == 0)
    {
        myIndexLen = -1;
        return;
    }

    //double curStamp = myRangeStamp;
    // If any field is not loaded, queue for loading and exit.
    foreach(Field* f, myField)
    {
        if(f != NULL && !f->loaded)
        {
            f->getDimension()->dataset->load(f);
            return;
        }
    }

    if(Dataset::useDoublePrecision()) filterKernel<double>(ti->getTimestamp());
    else filterKernel<float>(ti->getTimestamp());
}

///////////////////////////////////////////////////////////////////////////////
void Filter::update()
{
}

///////////////////////////////////////////////////////////////////////////////
GpuBuffer* Filter::getIndexBuffer(const DrawContext& dc)
{
    if(myGpuBuffer(dc) == NULL)
    {
        myGpuBuffer(dc) = dc.gpuContext->createVertexBuffer();
        myGpuBuffer(dc)->setType(GpuBuffer::IndexData);
    }

    // Do we need to update the gpu buffer with new data?
    if(myGpuBuffer.stamp(dc) < myIndexStamp)
    {
        myLock.lock();
        myGpuBuffer.stamp(dc) = myIndexStamp;
        myGpuBuffer(dc)->setData(myIndexLen * sizeof(uint), myIndices);
        myLock.unlock();
    }
    return myGpuBuffer(dc);

}
