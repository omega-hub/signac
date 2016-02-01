#include <omega.h>
#include <omega/PythonInterpreterWrapper.h>

#include "CsvLoader.h"
#include "Dataset.h"
#include "Scatterplot.h"

using namespace omega;

List<Plot*> plots;

///////////////////////////////////////////////////////////////////////////////
// Render pass for plots.
class PlotRenderPass : public RenderPass
{
public:
    PlotRenderPass(Renderer* r) : RenderPass(r, "PlotRenderPass")
    {}

    void render(Renderer* client, const DrawContext& context)
    {
        client->getRenderer()->beginDraw2D(context);
        foreach(Plot* p, plots)
        {
            p->render(context);
        }
        client->getRenderer()->endDraw();
    }

};

///////////////////////////////////////////////////////////////////////////////
// Signac Engine module, gets registered when module is imported, takes care
// of updating plots etc.
class ModuleCore : public EngineModule
{
public:
    ModuleCore() : EngineModule("signac")
    {
        ModuleServices::addModule(this);
        requestOpenGLProfile(EngineModule::CoreProfile);
    }

    void update(const UpdateContext& context)
    {
        foreach(Plot* p, plots)
        {
            p->update();
        }
    }

    virtual void initializeRenderer(Renderer* r)
    {
        r->addRenderPass(new PlotRenderPass(r));
    }
};

ModuleCore* moduleCore;

BOOST_PYTHON_MODULE(signac)
{
    PYAPI_ENUM(FieldInfo::FieldType, FieldType)
        PYAPI_ENUM_VALUE(FieldInfo, Float)
        ;

    PYAPI_REF_BASE_CLASS(FieldInfo)
        PYAPI_PROPERTY(FieldInfo, id)
        PYAPI_PROPERTY(FieldInfo, label)
        PYAPI_PROPERTY(FieldInfo, floatRangeMax)
        PYAPI_PROPERTY(FieldInfo, floatRangeMin)
        ;

    PYAPI_REF_BASE_CLASS(Field)
        PYAPI_REF_GETTER(Field, getInfo)
        PYAPI_PROPERTY(Field, loaded)
        PYAPI_METHOD(Field, range)
        ;

    PYAPI_REF_BASE_CLASS(Loader)
        ;

    PYAPI_REF_CLASS_WITH_CTOR(CsvLoader, Loader)
        ;
        
    PYAPI_REF_BASE_CLASS_WITH_CTOR(Dataset)
        PYAPI_METHOD(Dataset, open)
        PYAPI_REF_GETTER(Dataset, addField)
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

    moduleCore = new ModuleCore();
}