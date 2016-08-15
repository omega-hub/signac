#include "signac.h"
#include "PointCloud.h"
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
        bd->va(dc)->setAttributeBinding(VA_FILTER, 0, "filter");
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

///////////////////////////////////////////////////////////////////////////////
PointCloud::PointCloud(const String& name) : NodeComponent(),
myVisible(true)
{
    float maxf = numeric_limits<float>::max();
    float minf = -numeric_limits<float>::max();
    //myBBox.setExtents(Vector3f(minf, minf, minf), Vector3f(maxf, maxf, maxf));
    myMinDataBounds = Vector4f(maxf, maxf, maxf, maxf);
    myMaxDataBounds = Vector4f(minf, minf, minf, minf);
    myProgramParams = new ProgramParams();

    myBatchDrawStat = Stat::create(ostr("%1% batches", %name), StatsManager::Primitive);
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

        // Batch bounding box is not ready at this point.
        //myBBox.merge(batch->getBoundingBox());
    }

    refreshFields();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setVectorData(Dimension* x, Dimension* y, Dimension* z)
{
    myDataX = x;
    myDataY = y;
    myDataZ = z;
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setData(Dimension* fi)
{ 
    myData = fi; 
    refreshFields(); 
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setSize(Dimension* fi) 
{ 
    mySize = fi; 
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFilter(Dimension* fi) 
{ 
    myFilter = fi; 
    refreshFields(); 
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::refreshFields()
{
    foreach(PointBatch* b, myBatches) b->refreshFields();
    if(myProgram != NULL)
    {
        if(mySize != NULL) myProgram->define("SIZE_MODE", "1");
        else myProgram->define("SIZE_MODE", "0");
        if(myFilter != NULL) myProgram->define("FILTER_MODE", "1");
        else myProgram->define("FILTER_MODE", "0");
    }
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::draw(const DrawContext& c)
{
    if(!isVisible()) return;
    myProgram->setParams(c, myProgramParams);

    Rectf viewRect(-1, -1, 2, 2);

    int bc = 0;
    foreach(PointBatch* b, myBatches)
    {
        if(b->hasBoundingBox())
        {
            const AlignedBox3& bb = b->getBoundingBox();
            const Vector3f* corners = bb.getAllCorners();
            Transform3 xf = c.projection * c.modelview * getOwner()->getFullTransform();
            Vector2f vmin(2.0f, 2.0f);
            Vector2f vmax(-2.0f, -2.0f);
            for(int i = 0; i < 8; i++)
            {
                Vector4f ch(corners[i][0], corners[i][1], corners[i][2], 1.0f);
                Vector4f c = xf * ch;
                c = c / c.w();
                vmin = vmin.cwiseMin(Vector2f(c[0], c[1]));
                vmax = vmax.cwiseMax(Vector2f(c[0], c[1]));
            }
            float x = vmin[0];
            float y = vmin[1];
            float w = vmax[0] - vmin[0];
            float h = vmax[1] - vmin[1];
            Rectf br(x, y, w, h);

            if(br.intersects(viewRect))
            {
                b->draw(c);
                bc++;
            }
        }
        else
        {
            // If the batch drawable does not have a bounding box yet, just
            // draw it. Bounding box will be ready eventually.
            b->draw(c);
            bc++;
        }
    }

    myBatchDrawStat->addSample(bc);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setProgram(Program* p)
{
    myProgram = p;
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::update(const UpdateContext& ctx)
{
    SceneNode* sn = getOwner();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setPointScale(float scale)
{
    myProgramParams->pointScale = scale;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFilterBounds(float fmin, float fmax)
{
    myProgramParams->filterMax = fmax;
    myProgramParams->filterMin = fmin;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::normalizeFilterBounds(bool enabled)
{
    myProgramParams->normalizedFilterBounds = enabled;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setColor(const Color& c)
{
    myProgramParams->color = c;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFocusPosition(const Vector3f d)
{
    myProgramParams->focusPosition = d;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setDecimation(int dec)
{
    myProgramParams->decimation = dec;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setColormap(PixelData* colormap)
{
    myColormap = colormap;
}

///////////////////////////////////////////////////////////////////////////////
PointCloudView::PointCloudView():
myWidth(800),
myHeight(600),
myColormapperEnabled(false),
myUpdateChannelBounds(false)
{
    Signac::instance->addPointCloudView(this);
    myOutput = new PixelData(PixelData::FormatRgba, myWidth, myHeight);
}

///////////////////////////////////////////////////////////////////////////////
PointCloudView::~PointCloudView()
{
    Signac::instance->removePointCloudView(this);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::addPointCloud(PointCloud* pc)
{
    myPointClouds.push_back(pc);
}
///////////////////////////////////////////////////////////////////////////////
void PointCloudView::removePointCloud(PointCloud*pc)
{
    myPointClouds.remove(pc);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::setColormap(PixelData* colormap)
{
    myColormap = colormap;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::setColormapper(Program* p)
{
    myMappingProgram = p;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::draw(const DrawContext& c)
{
    // Initialize the texture and render target (if needed)
    if(myChannelRT(c) == NULL)
    {
        myChannelTexture(c) = c.gpuContext->createTexture();
        myChannelTexture(c)->initialize(
            myWidth, myHeight,
            Texture::Type2D,
            Texture::ChannelRGBA,
            Texture::FormatFloat);
        myChannelRT(c) = c.gpuContext->createRenderTarget(RenderTarget::RenderToTexture);
        myChannelRT(c)->setTextureTarget(myChannelTexture(c));

        myOutputRT(c) = c.gpuContext->createRenderTarget(RenderTarget::RenderOffscreen);
        myOutputRT(c)->setReadbackTarget(myOutput);

        float vertices[] = {
            // Pos      // Tex
            -1.0f, -1.0f, 0.0f, 1.0f,
            -1.0f, 1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 0.0f
        };
        myQuad(c) = c.gpuContext->createVertexArray();
        myQuad(c)->addBuffer(0, GpuBuffer::VertexData, 16 * sizeof(float), vertices);
        myQuad(c)->addAttribute(0, 0, "vertex", GpuBuffer::Float, false, 4, 0, 0);

        myChannelTextureDraw(c) = new GpuDrawCall();
        myChannelTextureDraw(c)->setVertexArray(myQuad(c));
        myChannelTextureDraw(c)->addTexture("channels", myChannelTexture(c));
        myChannelTextureDraw(c)->addTexture("colormap");
        myChannelTextureDraw(c)->primType = GpuDrawCall::PrimTriangleStrip;
        myChannelTextureDraw(c)->items = 4;
    }

    if(myWidth != myChannelTexture(c)->getWidth() ||
        myHeight != myChannelTexture(c)->getHeight())
    {
        myChannelTexture(c)->resize(myWidth, myHeight);
    }

    if(myColormapperEnabled)
    {
        if(myColormap != NULL)
        {
            Texture* cm = myColormap->getTexture(c);
            if(myColormapTexture(c) != cm)
            {
                myChannelTextureDraw(c)->setTexture("colormap", cm);
                myColormapTexture(c) = cm;
            }
        }
        // Render the point cloud to the output texture.
        myChannelRT(c)->bind();
        myChannelRT(c)->clear();
    }
    else
    {
        // Render the point cloud to the output texture.
        myOutputRT(c)->bind();
        myOutputRT(c)->clear();
    }
    // FOr now draw all batches no frustum culling
    foreach(PointCloud* pc, myPointClouds)
    {
        pc->draw(c);
    }
    if(myColormapperEnabled)
    {
        myChannelRT(c)->unbind();
    }
    else
    {
        myOutputRT(c)->unbind();
        myOutputRT(c)->readback();
    }
    if(myColormapperEnabled && !myMappingProgram.isNull())
    {
        myChannelTextureDraw(c)->setProgram(myMappingProgram->getGpuProgram(c));

        /// If an update of the channel bounds is requested, read back the 
        // channel texture and compute min/max
        if(myUpdateChannelBounds)
        {
            myChannelMax = -std::numeric_limits<float>::max();
            myChannelMin = std::numeric_limits<float>::max();
            myUpdateChannelBounds = false;
            size_t chdatasize = myWidth * myHeight * 4;
            float* chdata = new float[chdatasize];
            myChannelTexture(c)->readRawPixels((byte*)chdata, chdatasize);
            for(size_t i = 0; i < chdatasize; i += 4)
            {
                myChannelMax = std::max(myChannelMax, chdata[i]);
                myChannelMin = std::min(myChannelMin, chdata[i]);
            }
            delete chdata;
            ofmsg("channel texture bounds %1%  %2%", %myChannelMax %myChannelMin);
        }

        myMappingProgram->getDataBounds(c)->set(myChannelMin, myChannelMax);
        myOutputRT(c)->bind();
        myOutputRT(c)->clear();
        myChannelTextureDraw(c)->run();
        myOutputRT(c)->unbind();
        myOutputRT(c)->readback();
    }
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::resize(int width, int height)
{
    myWidth = width;
    myHeight = height;
    myOutput->resize(width, height);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::updateChannelBounds(bool useChannelTexture)
{
    // Get channel bounds
    float chmin = myPointClouds.front()->getData()->floatRangeMin;
    float chmax = myPointClouds.front()->getData()->floatRangeMax;
    foreach(PointCloud* pc, myPointClouds)
    {
        Dimension* d = pc->getData();
        if(d != NULL)
        {
            chmin = min(chmin, (float)d->floatRangeMin);
            chmax = max(chmax, (float)d->floatRangeMax);
        }
    }
    setChannelBounds(chmin, chmax);
    myUpdateChannelBounds = useChannelTexture;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloudView::setChannelBounds(float cmin, float cmax)
{
    myChannelMin = cmin;
    myChannelMax = cmax;
}
