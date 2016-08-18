#include <omega.h>
#include <omega/PythonInterpreterWrapper.h>

#include "signac.h"
#include "CsvLoader.h"
#include "BinaryLoader.h"
#include "Dataset.h"
#include "Hdf5Loader.h"
#include "FireLoader.h"
#include "Scatterplot.h"
#include "PointCloud.h"
#include "PointCloudView.h"

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
            foreach(PointCloudView* p, Signac::instance->myPointCloudViews)
            {
                p->draw(context);
            }
            client->getRenderer()->endDraw();
        }
    }

};

///////////////////////////////////////////////////////////////////////////////
Signac::Signac() : EngineModule("signac"),
    myWorkerThreads(4)
{
    ModuleServices::addModule(this);
    requestOpenGLProfile(EngineModule::CoreProfile);
}

///////////////////////////////////////////////////////////////////////////////
void Signac::dispose()
{
    if(!myWorkers.isNull())
    {
        omsg("[Signac::dispose] clearing worker queue");
        myWorkers->clearQueue();
        myWorkers->stop();
    }
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
void Signac::addPointCloudView(PointCloudView* pc)
{
    AutoLock al(myLock);
    myPointCloudViews.push_back(pc);
}

///////////////////////////////////////////////////////////////////////////////
void Signac::removePointCloudView(PointCloudView* pc)
{
    AutoLock al(myLock);
    myPointCloudViews.remove(pc);
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

///////////////////////////////////////////////////////////////////////////////
void Signac::signalFieldLoaded(Field* f)
{
    if(myFieldLoadedCommand.length() > 0)
    {
        PythonInterpreter* pi = SystemManager::instance()->getScriptInterpreter();
        String cmd = StringUtils::replaceAll(myFieldLoadedCommand, "%1%", f->getName());
        pi->queueCommand(cmd);
    }
}

///////////////////////////////////////////////////////////////////////////////
void Signac::addTask(WorkerTask* task)
{
    if(myWorkers.isNull())
    {
        myWorkers = new WorkerPool();
        myWorkers->start(myWorkerThreads);
    }
    myWorkers->queue(task);
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

    PYAPI_REF_CLASS_WITH_CTOR(Hdf5Loader, Loader)
        ;

    PYAPI_REF_CLASS_WITH_CTOR(FireLoader, Loader)
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

    PYAPI_REF_BASE_CLASS_WITH_CTOR(PointCloudView)
        PYAPI_METHOD(PointCloudView, addPointCloud)
        PYAPI_METHOD(PointCloudView, removePointCloud)
        PYAPI_METHOD(PointCloudView, resize)
        PYAPI_METHOD(PointCloudView, enableColormapper)
        PYAPI_METHOD(PointCloudView, setColormapper)
        PYAPI_METHOD(PointCloudView, setColormap)
        PYAPI_METHOD(PointCloudView, updateChannelBounds)
        PYAPI_METHOD(PointCloudView, setChannelBounds)
        PYAPI_METHOD(PointCloudView, setColormap)
        PYAPI_METHOD(PointCloudView, getChannelMin)
        PYAPI_METHOD(PointCloudView, getChannelMax)
        PYAPI_REF_GETTER(PointCloudView, getOutput)
        ;

    PYAPI_REF_CLASS(PointCloud, NodeComponent)
        PYAPI_STATIC_REF_GETTER(PointCloud, create)
        PYAPI_METHOD(PointCloud, setOptions)
        PYAPI_METHOD(PointCloud, setDimensions)
        PYAPI_METHOD(PointCloud, setData)
        PYAPI_METHOD(PointCloud, setVectorData)
        PYAPI_REF_GETTER(PointCloud, getDataX)
        PYAPI_REF_GETTER(PointCloud, getDataY)
        PYAPI_REF_GETTER(PointCloud, getDataZ)
        PYAPI_METHOD(PointCloud, setSize)
        PYAPI_METHOD(PointCloud, setFilter)
        PYAPI_METHOD(PointCloud, setFilterBounds)
        PYAPI_METHOD(PointCloud, normalizeFilterBounds)
        PYAPI_METHOD(PointCloud, setProgram)
        PYAPI_METHOD(PointCloud, setPointScale)
        PYAPI_METHOD(PointCloud, setColormap)
        PYAPI_METHOD(PointCloud, setColor)
        PYAPI_METHOD(PointCloud, setDecimation)
        PYAPI_METHOD(PointCloud, setFocusPosition)
        PYAPI_METHOD(PointCloud, setVisible)
        PYAPI_REF_GETTER(PointCloud, getColormap)
        PYAPI_REF_GETTER(PointCloud, getProgram)
        ;

    PYAPI_REF_BASE_CLASS(Program)
        PYAPI_METHOD(Program, setVertexShader)
        PYAPI_METHOD(Program, setFragmentShader)
        PYAPI_METHOD(Program, setGeometryShader)
        PYAPI_METHOD(Program, reload)
        PYAPI_METHOD(Program, define)
        PYAPI_METHOD(Program, clearDefines)
        ;

    PYAPI_REF_BASE_CLASS(ProgramParams);
    
    PYAPI_REF_BASE_CLASS_WITH_CTOR(Signac)
        PYAPI_STATIC_REF_GETTER(Signac, getInstance)
        PYAPI_REF_GETTER(Signac, addProgram)
        PYAPI_METHOD(Signac, setFieldLoadedCommand)
        PYAPI_METHOD(Signac, setWorkerThreads)
        ;

    Signac::instance = new Signac();
}