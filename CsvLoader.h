#ifndef __CSVLOADER_H__
#define __CSVLOADER_H__

#include "Loader.h"

using namespace omega;

///////////////////////////////////////////////////////////////////////////////
class CsvLoader : public Loader
{
public:
    ~CsvLoader();

    void open(const String& source);
    void load(Field* f);

private:
    String myFilename;
    WorkerPool myLoaderPool;
};
#endif
