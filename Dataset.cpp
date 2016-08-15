#include "Dataset.h"
#include "Loader.h"

bool Dataset::mysDoublePrecision = false;

///////////////////////////////////////////////////////////////////////////////
Dimension::Dimension():
dataset(NULL)
{
    floatRangeMax = -std::numeric_limits<float>::max();
    floatRangeMin = std::numeric_limits<float>::max();
}

///////////////////////////////////////////////////////////////////////////////
size_t Dimension::getElementSize()
{
    switch(type)
    {
    case Float: return Dataset::useDoublePrecision() ? 8 : 4;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
Field::Field(Dimension* info, const Domain& dom):
    myInfo(info),
    domain(dom),
    data(NULL),
    loaded(false),
    loading(false),
    stamp(0)
{
    boundMax = -std::numeric_limits<float>::max();
    boundMin = std::numeric_limits<float>::max();
}

///////////////////////////////////////////////////////////////////////////////
Dataset::Dataset(const String& name):
    myLoader(NULL),
    myName(name),
    myNumRecords(0)
{
}

///////////////////////////////////////////////////////////////////////////////
size_t Dataset::getNumRecords()
{ 
    if(myNumRecords == 0) myNumRecords = myLoader->getNumRecords(this);
    return myNumRecords;
}

///////////////////////////////////////////////////////////////////////////////
Dimension* Dataset::addDimension(const String& name, Dimension::Type type, int index, const String& label)
{
    //unsigned int index = (unsigned int)myDimensions.size();

    Dimension* fi = new Dimension();
    fi->dataset = this;
    fi->label = label;
    fi->id = name;
    fi->index = index;
    fi->type = type;

    myDimensions.push_back(fi);
    return fi;
}

///////////////////////////////////////////////////////////////////////////////
Field* Dataset::addField(Dimension* dim, const Domain& dom)
{
    Field* f = new Field(dim, dom);
    myFields.push_back(f);
    return f;
}

///////////////////////////////////////////////////////////////////////////////
Field* Dataset::findField(Dimension* dimension, const Domain& domain)
{
    foreach(Field* f, myFields)
    {
        if(f->getDimension() == dimension && f->domain == domain) return f;
    }
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
Field* Dataset::getOrCreateField(Dimension* dimension, const Domain& domain)
{
    Field* f = findField(dimension, domain);
    if(f == NULL) f = addField(dimension, domain);
    return f;
}

///////////////////////////////////////////////////////////////////////////////
void Dataset::setLoader(Loader* loader)
{
    myLoader = loader;
    myLoader->ref();
}

///////////////////////////////////////////////////////////////////////////////
void Dataset::load(Field* f)
{
    if(!f->loading && !f->loaded)
    {
        f->loading = true;
        //ofmsg("[Field::getGpuBuffer queue for load] field %1%", %f->getName());
        myLoader->load(f);
    }
}

///////////////////////////////////////////////////////////////////////////////
GpuBuffer* Field::getGpuBuffer(const DrawContext& dc)
{
    if(data == NULL)
    {
        myInfo->dataset->load(this);
        return NULL;
    }
    if(myGpuBuffer(dc) == NULL)
    {
        myGpuBuffer(dc) = dc.gpuContext->createVertexBuffer();
        myGpuBuffer(dc)->setType(GpuBuffer::VertexData);
        myGpuBuffer(dc)->setAttribute(0, 
            myInfo->dataset->useDoublePrecision() ? GpuBuffer::Double : GpuBuffer::Float);
        //ofmsg("[Field::getGpuBuffer create] field %1%", %getName());
    }

    // Do we need to update the gpu buffer with new data?
    if(myGpuBuffer.stamp(dc) < stamp)
    {
        lock.lock();
        myGpuBuffer.stamp(dc) = stamp;
        size_t sz = domain.length * myInfo->getElementSize() / domain.decimation;
        myGpuBuffer(dc)->setData(sz, data);
        //ofmsg("[Field::getGpuBuffer update] field %1% length %2%", %myInfo->id %length);
        lock.unlock();
    }
    return myGpuBuffer(dc);
}

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
