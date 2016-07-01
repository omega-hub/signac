#ifndef __SIGNAC__
#define __SIGNAC__

#include <omega.h>
#include "PointCloud.h"
#include "Program.h"
using namespace omega;


///////////////////////////////////////////////////////////////////////////////
// Signac Engine module, gets registered when module is imported, takes care
// of updating plots etc.
class Signac : public EngineModule
{
public:
    static Signac* instance;

public:
    static Signac* getInstance() { return instance; }
    Signac();
    void update(const UpdateContext& context);
    virtual void initializeRenderer(Renderer* r);
    void addPointCloud(PointCloud* pc);
    void removePointCloud(PointCloud* pc);

    Program* addProgram(const String& name);
    Program* getProgram(const String& name);

    PointCloud::List& getPointClouds() { return myPointClouds; }
    Program::List& getPrograms() { return myPrograms; }

private:
    PointCloud::List myPointClouds;
    Program::List myPrograms;
    Lock myLock;

};

#endif