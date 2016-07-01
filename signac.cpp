#include <omega.h>
#include <omega/PythonInterpreterWrapper.h>

#include "signac.h"
#include "CsvLoader.h"
#include "BinaryLoader.h"
#include "Dataset.h"
#include "Scatterplot.h"
#include "PointCloud.h"

using namespace omega;

List<Plot*> plots;

///////////////////////////////////////////////////////////////////////////////
// Render pass for plots.
class SignacRenderPass : public RenderPass
{
public:
    SignacRenderPass(Renderer* r) : RenderPass(r, "SignacRenderPass")
    {}

    void render(Renderer* client, const DrawContext& context)
    {
        foreach(Program* p, Signac::instance->getPrograms())
        {
            p->prepare(context);
        }

        // Draw 2D plots
        if(context.task == DrawContext::OverlayDrawTask)
        {
            client->getRenderer()->beginDraw2D(context);
            foreach(Plot* p, plots)
            {
                p->render(context);
            }
            client->getRenderer()->endDraw();
        }

        // Draw 3D point clouds
        if(context.task == DrawContext::SceneDrawTask)
        {
            client->getRenderer()->beginDraw3D(context);
            foreach(PointCloud* p, Signac::instance->getPointClouds())
            {
                p->draw(context);
            }
            client->getRenderer()->endDraw();
        }
    }

};

///////////////////////////////////////////////////////////////////////////////
Signac::Signac() : EngineModule("signac")
{
    ModuleServices::addModule(this);
    requestOpenGLProfile(EngineModule::CoreProfile);
}

///////////////////////////////////////////////////////////////////////////////
void Signac::update(const UpdateContext& context)
{
    foreach(Plot* p, plots)
    {
        p->update();
    }
}

///////////////////////////////////////////////////////////////////////////////
void Signac::initializeRenderer(Renderer* r)
{
    r->addRenderPass(new SignacRenderPass(r));
}

///////////////////////////////////////////////////////////////////////////////
void Signac::addPointCloud(PointCloud* pc)
{
    AutoLock al(myLock);
    myPointClouds.push_back(pc);
}

///////////////////////////////////////////////////////////////////////////////
void Signac::removePointCloud(PointCloud* pc)
{
    AutoLock al(myLock);
    myPointClouds.remove(pc);
}

///////////////////////////////////////////////////////////////////////////////
Program* Signac::addProgram(const String& name)
{
    Program* p = new Program(name);
    myPrograms.push_back(p);
    return p;
}

///////////////////////////////////////////////////////////////////////////////
Program* Signac::getProgram(const String& name)
{
    foreach(Program* p, myPrograms)
    {
        if(p->getName() == name) return p;
    }
    return NULL;
}


Signac* Signac::instance = NULL;

BOOST_PYTHON_MODULE(signac)
{
    PYAPI_ENUM(Dimension::Type, DimensionType)
        PYAPI_ENUM_VALUE(Dimension, Float)
        ;

    PYAPI_REF_BASE_CLASS(Dimension)
        PYAPI_PROPERTY(Dimension, id)
        PYAPI_PROPERTY(Dimension, label)
        PYAPI_PROPERTY(Dimension, floatRangeMax)
        PYAPI_PROPERTY(Dimension, floatRangeMin)
        ;

    PYAPI_REF_BASE_CLASS(Field)
        PYAPI_REF_GETTER(Field, getDimension)
        PYAPI_PROPERTY(Field, loaded)
        PYAPI_METHOD(Field, range)
        ;

    PYAPI_REF_BASE_CLASS(Loader)
        PYAPI_METHOD(Loader, open)
        ;

    PYAPI_REF_CLASS_WITH_CTOR(CsvLoader, Loader)
        ;
        
    PYAPI_REF_CLASS_WITH_CTOR(BinaryLoader, Loader)
        ;

    PYAPI_REF_BASE_CLASS(Dataset)
        PYAPI_STATIC_REF_GETTER(Dataset, create)
        PYAPI_METHOD(Dataset, setLoader)
        PYAPI_REF_GETTER(Dataset, addField)
        PYAPI_REF_GETTER(Dataset, addDimension)
        PYAPI_STATIC_METHOD(Dataset, useDoublePrecision)
        PYAPI_STATIC_METHOD(Dataset, setDoublePrecision)
        ;

    PYAPI_REF_BASE_CLASS_WITH_CTOR(Filter)
        PYAPI_METHOD(Filter, setField)
        PYAPI_METHOD(Filter, setRange)
        PYAPI_METHOD(Filter, setNormalizedRange)
        ;

    PYAPI_REF_BASE_CLASS(PlotBrush)
        PYAPI_STATIC_REF_GETTER(PlotBrush, create)
        PYAPI_METHOD(PlotBrush, setFilter)
        PYAPI_METHOD(PlotBrush, setColor)
        PYAPI_METHOD(PlotBrush, setEnabled)
        PYAPI_METHOD(PlotBrush, isEnabled)
        PYAPI_METHOD(PlotBrush, refresh)
        PYAPI_METHOD(PlotBrush, setBlend)
        ;

    PYAPI_REF_BASE_CLASS_WITH_CTOR(Plot)
        PYAPI_METHOD(Plot, setX)
        PYAPI_METHOD(Plot, setY)
        PYAPI_REF_GETTER(Plot, getX)
        PYAPI_REF_GETTER(Plot, getY)
        PYAPI_METHOD(Plot, setSize)
        PYAPI_METHOD(Plot, refresh)
        PYAPI_REF_GETTER(Plot, getPixels)
        PYAPI_METHOD(Plot, setBrush)
        PYAPI_REF_GETTER(Plot, getBrush)
        ;

    PYAPI_REF_CLASS_WITH_CTOR(PointCloud, NodeComponent)
        PYAPI_METHOD(PointCloud, setOptions)
        PYAPI_METHOD(PointCloud, setDimensions)
        PYAPI_METHOD(PointCloud, setData)
        PYAPI_METHOD(PointCloud, setProgram)
        PYAPI_REF_GETTER(PointCloud, getProgram)
        ;

    PYAPI_REF_BASE_CLASS(ProgramParams)
        PYAPI_PROPERTY(ProgramParams, pointScale)
        PYAPI_METHOD(ProgramParams, dataFilter)
        ;

    PYAPI_REF_BASE_CLASS(Program)
        PYAPI_METHOD(Program, setVertexShader)
        PYAPI_METHOD(Program, setFragmentShader)
        PYAPI_METHOD(Program, setGeometryShader)
        PYAPI_METHOD(Program, setColormapShader)
        PYAPI_METHOD(Program, setParams)
        PYAPI_REF_GETTER(Program, getParams)
        PYAPI_METHOD(Program, reload)
        PYAPI_METHOD(Program, define)
        PYAPI_METHOD(Program, clearDefines)
        ;

        PYAPI_REF_BASE_CLASS_WITH_CTOR(Signac)
        PYAPI_STATIC_REF_GETTER(Signac, getInstance)
        PYAPI_REF_GETTER(Signac, addProgram)
        ;

    Signac::instance = new Signac();
}