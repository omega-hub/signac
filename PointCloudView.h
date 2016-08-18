#ifndef __POINT_CLOUD_VIEW__
#define __POINT_CLOUD_VIEW__

#include <omega.h>

#include "Dataset.h"
#include "PointCloud.h"
#include "Program.h"

using namespace omega;

class PointCloud;
class PointBatch;

///////////////////////////////////////////////////////////////////////////////
class PointCloudView : public ReferenceType
{
public:
    PointCloudView();
    ~PointCloudView();

    void addPointCloud(PointCloud* pc);
    void removePointCloud(PointCloud* pc);
    void draw(const DrawContext& dc);

    void enableColormapper(bool enabled) { myColormapperEnabled = enabled; }
    void setColormapper(Program* p);
    Program* getMappingProgram() { return myMappingProgram; }
    void setColormap(PixelData* colormap);
    PixelData* getColormap() { return myColormap; }

    void resize(int width, int height);

    PixelData* getOutput() { return myOutput; }

    void updateChannelBounds(bool useChannelTexture);
    void setChannelBounds(float cmin, float cmax);
    float getChannelMin() { return myChannelMin;  }
    float getChannelMax() { return myChannelMax; }

private:
    PointCloud::List myPointClouds;

    Ref<PixelData> myOutput;
    bool myColormapperEnabled;
    Ref<PixelData> myColormap;
    GpuRef<Texture> myColormapTexture;

    // Image render
    GpuRef<RenderTarget> myChannelRT;
    GpuRef<RenderTarget> myOutputRT;
    GpuRef<Texture> myChannelTexture;
    GpuRef<GpuArray> myQuad;
    GpuRef<GpuDrawCall> myChannelTextureDraw;
    Ref<Program> myMappingProgram;
    int myWidth;
    int myHeight;

    float myChannelMin;
    float myChannelMax;
    bool myUpdateChannelBounds;
};

#endif