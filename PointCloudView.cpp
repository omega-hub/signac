#include "signac.h"
#include "PointCloudView.h"

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
