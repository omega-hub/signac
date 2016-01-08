#ifndef __SCATTERPLOT_H__
#define __SCATTERPLOT_H__
#include <omega.h>
#include "Dataset.h"

using namespace omega;

class Plot;

///////////////////////////////////////////////////////////////////////////////
//! Drawing options for a subset of data on a plot (determined by a filter)
class PlotBrush: public ReferenceType
{
public:
    static PlotBrush* create(const String& type) { return new PlotBrush(type); }

    void refresh() { myStamp = otimestamp(); }
    void setFilter(Filter* f);
    Filter* getFilter() { return myFilter; }

    void setEnabled(bool enabled) { myEnabled = enabled; }
    bool isEnabled() { return myEnabled;  }

    void setColor(const Color& c) { myColor = c; }
    void setBlend(bool blend) { myBlend = blend; }

    void draw(Plot* p, const DrawContext& dc);

    double getTimestamp();

private:
    PlotBrush(const String& type);

private:
    String myType;
    double myStamp;
    bool myEnabled;
    Color myColor;
    bool myBlend;

    Ref<Filter> myFilter;
    GpuRef<GpuDrawCall> myDrawCall;

    GpuRef<VertexArray> myVA;

    // Uniforms
    GpuRef<Uniform> myUMinX;
    GpuRef<Uniform> myUMinY;
    GpuRef<Uniform> myUMaxX;
    GpuRef<Uniform> myUMaxY;
    GpuRef<Uniform> myUColor;
};

///////////////////////////////////////////////////////////////////////////////
class Plot : public ReferenceType
{
public:
    static const int MaxBrushes = 4;

public:
    Plot();
    ~Plot();

    void refresh() { myStamp = otimestamp(); }
    void setSize(int width, int height);

    void setX(Field* x);
    void setY(Field* y);
    Field* getX() { return myX; }
    Field* getY() { return myY; }

    int getWidth() { return myWidth; }
    int getHeight() { return myHeight; }

    void update();

    PlotBrush* getBrush(int id) { return myBrush[id]; }
    void setBrush(int id, PlotBrush* b) { myBrush[id] = b; }
    void render(const DrawContext& context);

    PixelData* getPixels() { return myPixels; }

private:
    double myStamp;
    int myWidth;
    int myHeight;

    Ref<PixelData> myPixels;

    Ref<Field> myX;
    Ref<Field> myY;

    GpuRef<RenderTarget> myRenderTarget;
    
    Ref<PlotBrush> myBrush[MaxBrushes];
};
#endif