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
