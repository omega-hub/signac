#include "Scatterplot.h"
#include "omega/glheaders.h"
extern List<Plot*> plots;

///////////////////////////////////////////////////////////////////////////////
PlotBrush::PlotBrush(const String& type):
    myType(type),
    myStamp(0),
    myEnabled(true),
    myBlend(true)
{

}

///////////////////////////////////////////////////////////////////////////////
void PlotBrush::setFilter(Filter* f)
{
    myFilter = f;
}

///////////////////////////////////////////////////////////////////////////////
double PlotBrush::getTimestamp()
{
    return max(myStamp, myFilter != NULL ? myFilter->getIndexStamp() : 0);
}

///////////////////////////////////////////////////////////////////////////////
void PlotBrush::draw(Plot* plot, const DrawContext& dc)
{
    // Initialize the texture and render target (if needed)
    if(myDrawCall(dc) == NULL)
    {
        GpuProgram* p = dc.gpuContext->createProgram();
        p->setShader(GpuProgram::VertexShader, "signac/shaders/" + myType + ".vert");
        p->setShader(GpuProgram::FragmentShader, "signac/shaders/" + myType + ".frag");

        myVA(dc) = dc.gpuContext->createVertexArray();
        myVA(dc)->setAttributeBinding(0, 0, "x");
        myVA(dc)->setAttributeBinding(1, 0, "y");

        myDrawCall(dc) = new GpuDrawCall(p);
        myDrawCall(dc)->setVertexArray(myVA(dc));
        myDrawCall(dc)->primType = GpuDrawCall::PrimPoints;

        myUMinX(dc) = myDrawCall(dc)->addUniform("xmin");
        myUMaxX(dc) = myDrawCall(dc)->addUniform("xmax");
        myUMinY(dc) = myDrawCall(dc)->addUniform("ymin");
        myUMaxY(dc) = myDrawCall(dc)->addUniform("ymax");
        myUColor(dc) = myDrawCall(dc)->addUniform("color");
    }

    Field* fx = plot->getX();
    Field* fy = plot->getY();

    VertexBuffer* xgpubuf = fx->getGpuBuffer(dc);
    VertexBuffer* ygpubuf = fy->getGpuBuffer(dc);

    if(xgpubuf != NULL && ygpubuf != NULL)
    {
        myVA(dc)->setBuffer(0, xgpubuf);
        myVA(dc)->setBuffer(1, ygpubuf);

        // Set ranges
        myUMinX(dc)->set(fx->getInfo()->floatRangeMin);
        myUMinY(dc)->set(fy->getInfo()->floatRangeMin);
        myUMaxX(dc)->set(fx->getInfo()->floatRangeMax);
        myUMaxY(dc)->set(fy->getInfo()->floatRangeMax);

        //glEnable(GL_BLEND);
        glEnable(GL_PROGRAM_POINT_SIZE);
        if(myBlend)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        }
        else
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        }

        if(myFilter != NULL && myFilter->getFilteredLength() != -1)
        {
            myVA(dc)->setBuffer(2, myFilter->getIndexBuffer(dc));
            myDrawCall(dc)->items = myFilter->getFilteredLength();
            myDrawCall(dc)->run();
        }
        else
        {
            myDrawCall(dc)->items = fx->length;
            myDrawCall(dc)->run();
        }
        oassert(!oglError);
    }
}


///////////////////////////////////////////////////////////////////////////////
Plot::Plot():
myStamp(0)
{
    plots.push_back(this);
    myPixels = new PixelData(PixelData::FormatRgba, 200, 200);
    setBrush(0, PlotBrush::create("base"));
}

///////////////////////////////////////////////////////////////////////////////
Plot::~Plot()
{
    plots.remove(this);
}

///////////////////////////////////////////////////////////////////////////////
void Plot::setSize(int width, int height)
{
    if((width != myWidth || height != myHeight) &&
        (width > 0 && height > 0))
    {
        myWidth = width;
        myHeight = height;
        myPixels->resize(myWidth, myHeight);
        refresh();
    }
}

///////////////////////////////////////////////////////////////////////////////
void Plot::setX(Field* x)
{
    myX = x;
    myX->dataset->load(myX);
    refresh();
}

///////////////////////////////////////////////////////////////////////////////
void Plot::setY(Field* y)
{
    myY = y;
    myY->dataset->load(myY);
    refresh();
}

///////////////////////////////////////////////////////////////////////////////
void Plot::render(const DrawContext& dc)
{
    // Initialize the texture and render target (if needed)
    if(myRenderTarget(dc) == NULL)
    {
        myRenderTarget(dc) = dc.gpuContext->createRenderTarget(RenderTarget::RenderOffscreen);
        myRenderTarget(dc)->setReadbackTarget(myPixels);
    }

    // If we don't have attached fields, we don't need to render.
    if(myX == NULL || myY == NULL) return;

    // If the render target stamp is up-to-date, we don't need to render.
    double fs = 0;
    for(int i = 0; i < MaxBrushes; i++)
    {
        if(myBrush[i] != NULL) fs = max(fs, myBrush[i]->getTimestamp());
    }
    double rts = myRenderTarget.stamp(dc);
    if(rts >= myX->stamp && 
        rts >= myY->stamp &&
        rts >= myStamp &&
        rts >= fs) return;

    myRenderTarget.stamp(dc) = otimestamp();

    myRenderTarget(dc)->bind();
    myRenderTarget(dc)->clear();
    for(int i = 0; i < MaxBrushes; i++)
    {
        if(myBrush[i] != NULL && myBrush[i]->isEnabled())
        {
            myBrush[i]->draw(this, dc);
        }
    }
    myRenderTarget(dc)->unbind();
    myRenderTarget(dc)->readback();
}

///////////////////////////////////////////////////////////////////////////////
void Plot::update()
{
}
