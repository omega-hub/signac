#include "signac.h"
#include "PointCloud.h"
#include "Loader.h"
#include "omega/glheaders.h"

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
        for(int i = 0; i < PointCloud::MaxDataDimensions; i++)
        {
            Dimension* dim = myOwner->getData(i);
            if(dim != NULL) bd->data[i] = ds->getOrCreateField(dim, d);
            else bd->data[i] = NULL;
        }
    }
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
        bd->va(dc)->setAttributeBinding(0, 0, "x");
        bd->va(dc)->setAttributeBinding(1, 0, "y");
        bd->va(dc)->setAttributeBinding(2, 0, "z");
        bd->va(dc)->setAttributeBinding(3, 0, "data0");

        bd->drawCall(dc) = new GpuDrawCall(p->getGpuProgram(dc));
        bd->drawCall(dc)->setVertexArray(bd->va(dc));
        bd->drawCall(dc)->primType = GpuDrawCall::PrimPoints;

        //myUMinX(dc) = myDrawCall(dc)->addUniform("xmin");
        //myUMaxX(dc) = myDrawCall(dc)->addUniform("xmax");
        //myUMinY(dc) = myDrawCall(dc)->addUniform("ymin");
        //myUMaxY(dc) = myDrawCall(dc)->addUniform("ymax");
        //myUColor(dc) = myDrawCall(dc)->addUniform("color");
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

    bool hasData[] = { false, false, false, false };
    GpuBuffer* databuf[] = { NULL, NULL, NULL, NULL };
    for(int i = 0; i < 4; i++)
    {
        Field* f = bd->data[i];
        if(f != NULL)
        {
            hasData[i] = true;
            databuf[i] = f->getGpuBuffer(dc);
            readyToDraw &= (databuf[i] != NULL);
        }
    }

    if(readyToDraw)
    {
        bd->va(dc)->setBuffer(0, xgpubuf);
        bd->va(dc)->setBuffer(1, ygpubuf);
        bd->va(dc)->setBuffer(2, zgpubuf);
        for(int i = 0; i < 4; i++)
        {
            if(hasData[i]) 
            {
                bd->va(dc)->setBuffer(3 + i, databuf[i]);
                Dimension* dim = bd->data[i]->getDimension();
                p->getDataBounds(0, dc)->set(dim->floatRangeMin, dim->floatRangeMax);
            }
        }

        // Set ranges
        //Dimension* dmx = fx->getDimension();
        //Dimension* dmy = fy->getDimension();
        //Dimension* dmy = fy->getDimension();

        //myUMinX(dc)->set(dmx->floatRangeMin);
        //myUMinY(dc)->set(dmy->floatRangeMin);
        //myUMaxX(dc)->set(dmx->floatRangeMax);
        //myUMaxY(dc)->set(dmy->floatRangeMax);

        Transform3 mvmat = dc.modelview * myOwner->getOwner()->getFullTransform();

        p->getMVMatrix(dc)->set(mvmat);
        p->getMVPMatrix(dc)->set(mvmat * dc.projection);

        glEnable(GL_BLEND);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_PROGRAM_POINT_SIZE);
        //if(myBlend)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        //else
        //{
        //    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //
        //}

        //if(myFilter != NULL && myFilter->getFilteredLength() != -1)
        //{
        //    myVA(dc)->setBuffer(2, myFilter->getIndexBuffer(dc));
        //    myDrawCall(dc)->items = myFilter->getFilteredLength();
        //    myDrawCall(dc)->run();
        //}
        //else
        //{
        bd->drawCall(dc)->items = static_cast<uint>(l);
        bd->drawCall(dc)->run();
        //}
        oassert(!oglError);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool PointCloud::setOptions(const String& options)
{
    LOD::parse(options, &myLODLevels, &myPointsPerBatch);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PointCloud::setDimensions(Dimension* x, Dimension* y, Dimension* z)
{
    myX = x;
    myY = y;
    myZ = z;

    // Dimensions must all be from the same dataset
    if(myX->dataset != myY->dataset || 
        myX->dataset != myZ->dataset || 
        myY->dataset != myZ->dataset) 
    {
        owarn("PointCloud dimensions must all be from the same dataset");
        return false;
    }

    myDataset = myX->dataset;

    size_t numRecords = myDataset->getNumRecords();
    if(numRecords == 0) return false;

    ofmsg("[BinaryPointsLoader] Total Points: <%1%>   Points per batch: <%2%>",
        %numRecords
        %myPointsPerBatch);

    // Iterate for each batch
    for(size_t start = 0; start <= numRecords; start += myPointsPerBatch)
    {
        PointBatch* batch = new PointBatch(this);
        myBatches.push_back(batch);

        // Create LOD groups for each batch
        foreach(LOD& ll, myLODLevels)
        {
            batch->addDrawable(&ll, start, myPointsPerBatch);
        }

        myBBox.merge(batch->getBoundingBox());
    }

    refreshFields();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::refreshFields()
{
    foreach(PointBatch* b, myBatches) b->refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::onAttached(SceneNode* sn)
{ 
    Signac::instance->addPointCloud(this);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::onDetached(SceneNode* sn)
{
    Signac::instance->removePointCloud(this);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::draw(const DrawContext& c)
{
    // Initialize the texture and render target (if needed)
    if(myRenderTarget(c) == NULL)
    {
        myRenderOutput(c) = c.gpuContext->createTexture();
        myRenderOutput(c)->initialize(
            800, 600, 
            Texture::TypeRectangle, 
            Texture::ChannelRGBA, 
            Texture::FormatFloat);
        myRenderTarget(c) = c.gpuContext->createRenderTarget(RenderTarget::RenderToTexture);
        myRenderTarget(c)->setTextureTarget(myRenderOutput(c));
        /*
        float vertices[] = { 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
        myFullscreenQuad(c) = c.gpuContext->createVertexArray();
        myFullscreenQuad(c)->addBuffer(0, GpuBuffer::VertexData, 8 * sizeof(float), vertices);
        myFullscreenQuad(c)->addAttribute(0, 0, "vertex", GpuBuffer::Float, false, 2, 0, 0);
        //myFullscreenDraw(c) = new GpuDrawCall(myMappingProgram->getGpuProgram(c));
        myFullscreenDraw(c)->setVertexArray(myFullscreenQuad(c));
        myFullscreenDraw(c)->items = 4;
        myFullscreenDraw(c)->primType = GpuDrawCall::PrimTriangleStrip;*/
    }

    // Render the point cloud to the output texture.
    //myRenderTarget(c)->bind();
    //myRenderTarget(c)->clear();
    // FOr now draw all batches no frustum culling
    foreach(PointBatch* b, myBatches) b->draw(c);
    //myRenderTarget(c)->unbind();

    // Render quad.

}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setProgram(Program* p)
{
    myProgram = p;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::update(const UpdateContext& ctx)
{
    SceneNode* sn = getOwner();

}
