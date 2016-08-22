#include "signac.h"
#include "PointBatch.h"
#include "Loader.h"
#include "omega/glheaders.h"

#define VA_X 0
#define VA_Y 1
#define VA_Z 2
#define VA_DATA 3
#define VA_SIZE 4
#define VA_FILTER 5
#define VA_VECTOR_DATA_X 6
#define VA_VECTOR_DATA_Y 7
#define VA_VECTOR_DATA_Z 8

///////////////////////////////////////////////////////////////////////////////
bool LOD::parse(const String& options, Vector<LOD>* lodlevels, size_t* pointsPerBatch)
{
    // Parse options (format: 'pointsPerBatch dist:dec+')
    // where pointsPerBatch is the number of points for each LOD group 
    // at max LOD, and each distmin:distmax:dec pair is a LOD level with distance from 
    // eye and decimation level.
    Vector<String> args = StringUtils::split(options, " ");
    *pointsPerBatch = boost::lexical_cast<size_t>(args[0]);

    int mindec = 1000000;
    for(int i = 1; i < args.size(); i++)
    {
        Vector<String> lodargs = StringUtils::split(args[i], ":");
        LOD ll(
            boost::lexical_cast<int>(lodargs[0]),
            boost::lexical_cast<int>(lodargs[1]),
            boost::lexical_cast<int>(lodargs[2])
            );
        lodlevels->push_back(ll);
        if(ll.dec < mindec) mindec = ll.dec;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
PointBatch::PointBatch(PointCloud* owner) :
    myOwner(owner) {}

///////////////////////////////////////////////////////////////////////////////
void PointBatch::addDrawable(LOD* lod, size_t start, size_t length)
{
    // The first drawable added will be used to compute the bounds of this point
    // batch.
    if(myDrawables.empty())
    {
        float bounds[14];
        Loader* l = myOwner->getDataset()->getLoader();
        l->getBounds(Domain(start, length, lod->dec), bounds);

        // Extend the point cloud bounding box with this batch corners
        myBBox.merge(Vector3f(bounds[0], bounds[2], bounds[4]));
        myBBox.merge(Vector3f(bounds[1], bounds[3], bounds[5]));
    }
    myDrawables.push_back(new BatchDrawable(this, lod, start, length));
}

///////////////////////////////////////////////////////////////////////////////
void PointBatch::refreshFields()
{
    Dataset* ds = myOwner->getDataset();
    foreach(BatchDrawable* bd, myDrawables)
    {
        Domain d(bd->batchStart, bd->batchLength, bd->LOD->dec);
        bd->x = ds->getOrCreateField(myOwner->getX(), d);
        bd->y = ds->getOrCreateField(myOwner->getY(), d);
        bd->z = ds->getOrCreateField(myOwner->getZ(), d);

        Dimension* dim;
        
        dim = myOwner->getData();
        bd->data = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;
        dim = myOwner->getFilter();
        bd->filter = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;
        dim = myOwner->getSize();
        bd->size = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;

        dim = myOwner->getDataX();
        bd->datax = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;
        dim = myOwner->getDataY();
        bd->datay = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;
        dim = myOwner->getDataZ();
        bd->dataz = dim != NULL ? ds->getOrCreateField(dim, d) : NULL;
    }
}

///////////////////////////////////////////////////////////////////////////////
bool PointBatch::hasBoundingBox()
{
    BatchDrawable* bd = myDrawables.front();
    return bd != NULL && bd->x != NULL && bd->x->loaded &&
        bd->y != NULL && bd->y->loaded &&
        bd->z != NULL && bd->z->loaded;
}

///////////////////////////////////////////////////////////////////////////////
const AlignedBox3& PointBatch::getBoundingBox()
{
    if(hasBoundingBox())
    {
        BatchDrawable* bd = myDrawables.front();
        myBBox.setExtents(
            bd->x->boundMin,
            bd->y->boundMin,
            bd->z->boundMin,
            bd->x->boundMax,
            bd->y->boundMax,
            bd->z->boundMax);
    }
    return myBBox; 
}

///////////////////////////////////////////////////////////////////////////////
void PointBatch::draw(const DrawContext& dc)
{
    // For now just draw first LOD.
    BatchDrawable* bd = myDrawables.front();

    Program* p = myOwner->getProgram();
    // Initialize the texture and render target (if needed)
    if(bd->drawCall(dc) == NULL)
    {
        bd->va(dc) = dc.gpuContext->createVertexArray();
        bd->va(dc)->setAttributeBinding(VA_X, 0, "x");
        bd->va(dc)->setAttributeBinding(VA_Y, 0, "y");
        bd->va(dc)->setAttributeBinding(VA_Z, 0, "z");
        bd->va(dc)->setAttributeBinding(VA_DATA, 0, "data");
        bd->va(dc)->setAttributeBinding(VA_SIZE, 0, "size");
        bd->va(dc)->setAttributeBinding(VA_FILTER, 0, "datafilter");
        bd->va(dc)->setAttributeBinding(VA_VECTOR_DATA_X, 0, "datax");
        bd->va(dc)->setAttributeBinding(VA_VECTOR_DATA_Y, 0, "datay");
        bd->va(dc)->setAttributeBinding(VA_VECTOR_DATA_Z, 0, "dataz");

        bd->drawCall(dc) = new GpuDrawCall(p->getGpuProgram(dc));
        bd->drawCall(dc)->setVertexArray(bd->va(dc));
        bd->drawCall(dc)->primType = GpuDrawCall::PrimPoints;
    }

    bd->drawCall(dc)->setProgram(p->getGpuProgram(dc));

    if(myOwner->getColormap() != NULL)
    {
        Texture* cm = myOwner->getColormap()->getTexture(dc);
        if(bd->colormap(dc) != cm)
        {
            bd->drawCall(dc)->clearTextures();
            bd->drawCall(dc)->addTexture("colormap", cm);
        }
    }

    Field* fx = bd->x;
    Field* fy = bd->y;
    Field* fz = bd->z;
 
    size_t l = min(fx->numElements(), fy->numElements());
    l = min(l, fz->numElements());

    GpuBuffer* xgpubuf = fx->getGpuBuffer(dc);
    GpuBuffer* ygpubuf = fy->getGpuBuffer(dc);
    GpuBuffer* zgpubuf = fz->getGpuBuffer(dc);

    bool readyToDraw = xgpubuf != NULL && ygpubuf != NULL && zgpubuf != NULL;

    bool hasData = false;
    bool hasSize = false;
    bool hasFilter = false;
    bool hasVectorData = false;

    GpuBuffer* databuf = NULL;
    GpuBuffer* sizebuf = NULL;
    GpuBuffer* filterbuf = NULL;
    GpuBuffer* dataxbuf = NULL;
    GpuBuffer* dataybuf = NULL;
    GpuBuffer* datazbuf = NULL;

    // Data field
    Field* fd = bd->data;
    if(fd != NULL)
    {
        hasData = true;
        databuf = fd->getGpuBuffer(dc);
        readyToDraw &= (databuf != NULL);
    }

    // Size field
    Field* fs = bd->size;
    if(fs != NULL)
    {
        hasSize = true;
        sizebuf = fs->getGpuBuffer(dc);
        readyToDraw &= (sizebuf != NULL);
    }

    // Filter field
    Field* ff = bd->filter;
    if(ff != NULL)
    {
        hasFilter = true;
        filterbuf = ff->getGpuBuffer(dc);
        readyToDraw &= (filterbuf != NULL);
    }

    Field* fdx = bd->datax;
    if(fdx)
    {
        dataxbuf = fdx->getGpuBuffer(dc);
        readyToDraw &= (dataxbuf != NULL);
    }

    Field* fdy = bd->datay;
    if(fdy)
    {
        dataybuf = fdy->getGpuBuffer(dc);
        readyToDraw &= (dataybuf != NULL);
    }

    Field* fdz = bd->dataz;
    if(fdz)
    {
        datazbuf = fdz->getGpuBuffer(dc);
        readyToDraw &= (datazbuf != NULL);
    }

    if(fdx != NULL && fdy != NULL && fdz != NULL) hasVectorData = true;

    if(readyToDraw)
    {
        bd->va(dc)->setBuffer(VA_X, xgpubuf);
        bd->va(dc)->setBuffer(VA_Y, ygpubuf);
        bd->va(dc)->setBuffer(VA_Z, zgpubuf);

        if(hasData) 
        {
            bd->va(dc)->setBuffer(VA_DATA, databuf);
            Dimension* dim = bd->data->getDimension();
            p->getDataBounds(dc)->set(dim->floatRangeMin, dim->floatRangeMax);
        }
        if(hasSize)
        {
            bd->va(dc)->setBuffer(VA_SIZE, sizebuf);
        }
        if(hasFilter)
        {
            bd->va(dc)->setBuffer(VA_FILTER, filterbuf);
            Dimension* dim = bd->filter->getDimension();
            ProgramParams* pp = myOwner->myProgramParams;
            float fmin = pp->filterMin;
            float fmax = pp->filterMax;
            if(pp->normalizedFilterBounds)
            {
                float l = dim->floatRangeMax - dim->floatRangeMin;
                fmin = fmin * l + dim->floatRangeMin;
                fmax = fmax * l + dim->floatRangeMin;
            }
            p->getFilterBounds(dc)->set(fmin, fmax);
        }
        if(hasVectorData)
        {
            bd->va(dc)->setBuffer(VA_VECTOR_DATA_X, dataxbuf);
            bd->va(dc)->setBuffer(VA_VECTOR_DATA_Y, dataybuf);
            bd->va(dc)->setBuffer(VA_VECTOR_DATA_Z, datazbuf);
        }

        Transform3 mvmat = dc.modelview * myOwner->getOwner()->getFullTransform();

        p->getMVMatrix(dc)->set(mvmat);
        p->getMVPMatrix(dc)->set(mvmat * dc.projection);

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        bd->drawCall(dc)->items = static_cast<uint>(l);
        bd->drawCall(dc)->run();
    }
}
