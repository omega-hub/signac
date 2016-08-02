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
    friend class PointCloudView;
    friend class SignacRenderPass;
public:
    static Signac* instance;

public:
    static Signac* getInstance() { return instance; }
    Signac();
    void update(const UpdateContext& context);
    virtual void initializeRenderer(Renderer* r);

    Program* addProgram(const String& name);
    Program* getProgram(const String& name);

    Program::List& getPrograms() { return myPrograms; }

protected:
    void addPointCloudView(PointCloudView* pc);
    void removePointCloudView(PointCloudView* pc);

private:
    List<PointCloudView*> myPointCloudViews;
    Program::List myPrograms;
    Lock myLock;

};

#endif