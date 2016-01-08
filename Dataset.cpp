#include "Dataset.h"
#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
FieldInfo::FieldInfo()
{
    floatRangeMax = -std::numeric_limits<float>::max();
    floatRangeMin = std::numeric_limits<float>::max();
}

///////////////////////////////////////////////////////////////////////////////
Field::Field(FieldInfo* info, Dataset* ds):
    myInfo(info),
    dataset(ds),
    data(NULL),
    loaded(false),
    loading(false),
    stamp(0),
    length(0)
{

}

///////////////////////////////////////////////////////////////////////////////
Dataset::Dataset():
    myLoader(NULL)
{

}

///////////////////////////////////////////////////////////////////////////////
Field* Dataset::addField(const String& id, FieldInfo::FieldType type, String name)
{
    if(name == "") name = id;
    unsigned int index = myFields.size();

    FieldInfo* fi = new FieldInfo();
    fi->label = name;
    fi->id = id;
    fi->index = index;
    fi->type = type;

    Field* f = new Field(fi, this);
    myFields.push_back(f);
    return f;
}


///////////////////////////////////////////////////////////////////////////////
bool Dataset::open(const String& filename, Loader* loader)
{
    myLoader = loader;
    myLoader->ref();
    myFilename = filename;

    String path;
    if(DataManager::findFile(filename, path))
    {
        myLoader->open(filename);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
void Dataset::load(Field* f)
{
    if(!f->loading && !f->loaded)
    {
        f->loading = true;
        ofmsg("[Field::getGpuBuffer queue for load] field %1%", %f->getInfo()->id);
        myLoader->load(f);
    }
}

///////////////////////////////////////////////////////////////////////////////
VertexBuffer* Field::getGpuBuffer(const DrawContext& dc)
{
    if(data == NULL)
    {
        dataset->load(this);
        return NULL;
    }
    if(myGpuBuffer(dc) == NULL)
    {
        myGpuBuffer(dc) = dc.gpuContext->createVertexBuffer();
        myGpuBuffer(dc)->setType(VertexBuffer::VertexData);
        myGpuBuffer(dc)->setAttribute(0, VertexBuffer::Float);
        ofmsg("[Field::getGpuBuffer create] field %1%", %myInfo->id);
    }

    // Do we need to update the gpu buffer with new data?
    if(myGpuBuffer.stamp(dc) < stamp)
    {
        lock.lock();
        myGpuBuffer.stamp(dc) = stamp;
        myGpuBuffer(dc)->setData(length * myInfo->getElementSize(), data);
        //ofmsg("[Field::getGpuBuffer update] field %1%", %myInfo->id);
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
        myMin[index] = f->getInfo()->floatRangeMin;
        myMax[index] = f->getInfo()->floatRangeMax;

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
            f->dataset->load(f);
            return;
        }
    }

    uint sz = myField[0]->length;

    uint* indices = (uint*)malloc(sz * sizeof(uint));
    memset(indices, 0, sz * sizeof(uint));

    uint len = 0;
    for(uint i = 0; i < sz; i++)
    {
        // if the filter stamp was updated, we are processing stale data. exit now.
        if(myRangeStamp > ti->getTimestamp())
        {
            free(indices);
            return;
        }
        
        bool pass = true;
        for(uint j = 0; j < myNumFields; j++)
        {
            float* d = (float*)myField[j]->data;
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
    ofmsg("index generated - length = %1%", %myIndexLen);
    myIndexStamp = otimestamp();
    myLock.unlock();
}

///////////////////////////////////////////////////////////////////////////////
void Filter::update()
{
}

///////////////////////////////////////////////////////////////////////////////
VertexBuffer* Filter::getIndexBuffer(const DrawContext& dc)
{
    if(myGpuBuffer(dc) == NULL)
    {
        myGpuBuffer(dc) = dc.gpuContext->createVertexBuffer();
        myGpuBuffer(dc)->setType(VertexBuffer::IndexData);
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
