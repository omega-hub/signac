#ifndef __PROGRAM__
#define __PROGRAM__

#include <omega.h>
#include "Dataset.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class ProgramParams : public ReferenceType
{
public:
    static const int MaxDataDimensions = 4;
public:
    ProgramParams();

    void dataFilter(int index, float dmin, float dmax);


    float pointScale;
    float dataMax[MaxDataDimensions];
    float dataMin[MaxDataDimensions];
};

///////////////////////////////////////////////////////////////////////////////
class Program: public ReferenceType
{
public:
    typedef List< Ref<Program> > List;
public:
    Program(const String& name);
    String getName() { return myName; }
    void setVertexShader(const String& filename);
    void setFragmentShader(const String& filename);
    void setGeometryShader(const String& filename);
    void setColormapShader(const String& filename);

    bool prepare(const DrawContext& context);
    GpuProgram* getGpuProgram(const DrawContext& context);

    Uniform* getMVPMatrix(const DrawContext& c) { return myMVPMatrix(c); }
    Uniform* getMVMatrix(const DrawContext& c) { return myMVMatrix(c); }
    Uniform* getPMatrix(const DrawContext& c) { return myPMatrix(c); }
    Uniform* getDataBounds(int index, const DrawContext& c) { return myDataBounds[index](c); }
    Uniform* getDataFilter(int index, const DrawContext& c) { return myDataFilter[index](c); }

    ProgramParams* getParams() { return myParams; }
    void setParams(ProgramParams* p) { oassert(p);  myParams = p; }

    void reload() { myDirty = true; }

    void define(const String& name, const String& value);
    void clearDefines();

private:
    String myName;
    String myVertexShaderFilename;
    String myFragmentShaderFilename;
    String myGeometryShaderFilename;
    String myColormapShaderFilename;

    Dictionary<String, String> myDefines;

    GpuRef<GpuProgram> myProgram;
    GpuRef<Uniform> myMVPMatrix;
    GpuRef<Uniform> myMVMatrix;
    GpuRef<Uniform> myPMatrix;
    GpuRef<Uniform> myPointScale;
    GpuRef<Uniform> mySliceBounds;
    GpuRef<Uniform> myDataBounds[ProgramParams::MaxDataDimensions];
    GpuRef<Uniform> myDataFilter[ProgramParams::MaxDataDimensions];

    // Program Parameters & Uniforms
    Ref<ProgramParams> myParams;

    bool myDirty;
};

#endif