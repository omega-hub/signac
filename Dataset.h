#ifndef __DATASET_H__
#define __DATASET_H__
#include <omega.h>

using namespace omega;

class Dataset;

///////////////////////////////////////////////////////////////////////////////
struct Domain 
{
    Domain(size_t s = 0, size_t l = 0, int d = 0) : 
        start(s), 
        length(l), 
        decimation(d),
        streamid(-1),
        streamoffset(0) {}
    bool operator==(const Domain& rhs)
    {
        // Note, we do NOT compare streamid and streamoffset, they are internal
        // values.
        return start == rhs.start && 
            length == rhs.length && 
            decimation == rhs.decimation;
    }

    size_t start;
    size_t length;
    int decimation;

    // These values are used by multifile loaders to translate logical domain 
    // positions starts to file-specific values.
    int streamid;
    size_t streamoffset;
};

///////////////////////////////////////////////////////////////////////////////
class Dimension : public ReferenceType
{
public:
    Dimension();

    enum Type 
    {
        Float
    };

    Dataset* dataset;
    String id;
    String label;
    Type type;
    unsigned int index;

    double floatRangeMin;
    double floatRangeMax;

    size_t getElementSize();

};

class Dataset;

///////////////////////////////////////////////////////////////////////////////
class Field : public ReferenceType
{
public:
    Field(Dimension* info, const Domain& dom);

    char* data;
    bool loaded;
    bool loading;
    Domain domain;
    double stamp;
    double boundMin;
    double boundMax;

    Lock lock;

    Dimension* getDimension() { return myInfo; }
    GpuBuffer* getGpuBuffer(const DrawContext& dc);

    Vector2f range()
    {
        return Vector2f(myInfo->floatRangeMin, myInfo->floatRangeMax);
    }

    String getName()
    {
        return ostr("%1%<%2%,%3%,%4%>", %myInfo->id %domain.start %domain.length %domain.decimation);
    }

    size_t numElements()
    {
        return domain.length / domain.decimation;
    }

private:
    Ref<Dimension> myInfo;

    GpuRef<GpuBuffer> myGpuBuffer;
};

class Loader;

///////////////////////////////////////////////////////////////////////////////
class Dataset : public ReferenceType
{
public:
    static const int MaxFields = 128;
    static void setDoublePrecision(bool enabled) { mysDoublePrecision = enabled; }
    static bool useDoublePrecision() { return mysDoublePrecision; }
public:
    // Creation function for the python API
    static Dataset* create(const String& name) { return new Dataset(name); }

    Dataset(const String& name);

    const String& getName() { return myName; }

    Dimension* addDimension(const String& name, Dimension::Type type, int index, const String& label);
    Field* addField(Dimension* dimension, const Domain& domain);
    Field* findField(Dimension* dimension, const Domain& domain);
    Field* getOrCreateField(Dimension* dimension, const Domain& domain);

    void setLoader(Loader* loader);
    Loader* getLoader() { return myLoader; }
    size_t getNumRecords();

    void load(Field* f);

private:
    static bool mysDoublePrecision;

    typedef List< Ref<Field> > FieldList;
    typedef List< Ref<Dimension> > DimensionList;

    DimensionList myDimensions;
    FieldList myFields;
    String myFilename;
    Loader* myLoader;
    String myName;
    size_t myNumRecords;
};
#endif