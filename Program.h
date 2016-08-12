#ifndef __PROGRAM__
#define __PROGRAM__

#include <omega.h>
#include "Dataset.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class ProgramParams : public ReferenceType
{
public:
    ProgramParams();

    float pointScale;
    Vector3f focusPosition;
    float filterMax;
    float filterMin;
    bool normalizedFilterBounds;
    Color color;
    int decimation;
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

    bool prepare(const DrawContext& context);
    void setParams(const DrawContext& context, ProgramParams* p);
    GpuProgram* getGpuProgram(const DrawContext& context);

    Uniform* getMVPMatrix(const DrawContext& c) { return myMVPMatrix(c); }
    Uniform* getMVMatrix(const DrawContext& c) { return myMVMatrix(c); }
    Uniform* getPMatrix(const DrawContext& c) { return myPMatrix(c); }
    Uniform* getDataBounds(const DrawContext& c) { return myDataBounds(c); }
    Uniform* getFilterBounds(const DrawContext& c) { return myFilterBounds(c); }


    void reload() { myDirty = true; }

    void define(const String& name, const String& value);
    void clearDefines();

private:
    String myName;
    String myVertexShaderFilename;
    String myFragmentShaderFilename;
    String myGeometryShaderFilename;

    Dictionary<String, String> myDefines;

    GpuRef<GpuProgram> myProgram;
    GpuRef<Uniform> myVMatrix;
    GpuRef<Uniform> myMVPMatrix;
    GpuRef<Uniform> myMVMatrix;
    GpuRef<Uniform> myPMatrix;
    GpuRef<Uniform> myPointScale;
    GpuRef<Uniform> myFocusPosition;
    GpuRef<Uniform> mySliceBounds;
    GpuRef<Uniform> myDataBounds;
    GpuRef<Uniform> myFilterBounds;
    GpuRef<Uniform> myColor;
    GpuRef<Uniform> myDecimation;


    // Program Parameters & Uniforms
    Ref<ProgramParams> myParams;

    bool myDirty;
};

#endif