#include "Program.h"

///////////////////////////////////////////////////////////////////////////////
ProgramParams::ProgramParams():
    pointScale(0.001)
{
    filterMin = -numeric_limits<float>::max();
    filterMax = numeric_limits<float>::max();
    normalizedFilterBounds = false;
}

///////////////////////////////////////////////////////////////////////////////
Program::Program(const String& name) : myName(name)
{
    myParams = new ProgramParams();
    myDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
void Program::setVertexShader(const String& filename)
{
    myVertexShaderFilename = filename;
    myDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
void Program::setFragmentShader(const String& filename)
{
    myFragmentShaderFilename = filename;
    myDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
void Program::setGeometryShader(const String& filename)
{
    myGeometryShaderFilename = filename;
    myDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
void Program::setColormapShader(const String& filename)
{
    myColormapShaderFilename = filename;
    myDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
void Program::define(const String& name, const String& value)
{
    myDefines[name] = value;
    myDirty = true;
}

///////////////////////////////////////////////////////////////////////////////
void Program::clearDefines()
{
    myDefines.clear();
    myDirty = false;
}

///////////////////////////////////////////////////////////////////////////////
bool Program::prepare(const DrawContext& dc)
{
    if(myProgram(dc) == NULL)
    {
        GpuProgram* p = dc.gpuContext->createProgram();

        myMVPMatrix(dc) = p->addUniform("modelViewProjection");
        myMVMatrix(dc) = p->addUniform("modelView");
        myPMatrix(dc) = p->addUniform("projection");

        myPointScale(dc) = p->addUniform("pointScale");

        myDataBounds(dc) = p->addUniform("dataBounds");
        myFilterBounds(dc) = p->addUniform("filterBounds");

        myProgram(dc) = p;
    }

    if(myDirty)
    {
        // Prep define string
        String defs;
        typedef KeyValue<String, String> Item;
        foreach(Item item, myDefines)
        {
            defs.append(ostr("#define %1% %2%\n", %item.getKey() % item.getValue()));
        }

        GpuProgram* p = myProgram(dc);
        if(!myVertexShaderFilename.empty())
        {
            int c = 0;
            p->setShaderSource(GpuProgram::VertexShader, "#version 400\n", c++);
            p->setShaderSource(GpuProgram::VertexShader, defs, c++);
            p->setShader(GpuProgram::VertexShader, myVertexShaderFilename, c++);
        }
        if(!myFragmentShaderFilename.empty())
        {
            int c = 0;
            p->setShaderSource(GpuProgram::FragmentShader, "#version 400\n", c++);
            if(!myColormapShaderFilename.empty())
            {
                p->setShader(GpuProgram::FragmentShader, myColormapShaderFilename, c++);
            }
            p->setShaderSource(GpuProgram::FragmentShader, defs, c++);
            p->setShader(GpuProgram::FragmentShader, myFragmentShaderFilename, c++);
        }
        if(!myGeometryShaderFilename.empty())
        {
            int c = 0;
            p->setShaderSource(GpuProgram::GeometryShader, "#version 400\n", c++);
            p->setShaderSource(GpuProgram::GeometryShader, defs, c++);
            p->setShader(GpuProgram::GeometryShader, myGeometryShaderFilename, c++);
        }
        bool ok = p->build();
        oassert(ok);

        myDirty = false;
    }

    myMVPMatrix(dc)->set(dc.mvp);
    myMVMatrix(dc)->set(dc.modelview);
    myPMatrix(dc)->set(dc.projection);

    // Set other program parameters
    myPointScale(dc)->set(myParams->pointScale);
    myFilterBounds(dc)->set(myParams->filterMin, myParams->filterMax);


    return true;
}

///////////////////////////////////////////////////////////////////////////////
GpuProgram* Program::getGpuProgram(const DrawContext& dc)
{
    return myProgram(dc);
}
