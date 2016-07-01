#include "BinaryLoader.h"

///////////////////////////////////////////////////////////////////////////////
template<typename T>
void BinaryLoader::readXYZ(
    const String& filename,
    size_t readStart, size_t readLength, int decimation,
    Vector<Vector3f>* points, Vector<Vector4f>* colors,
    size_t* numPoints,
    Vector3f* pointmin,
    Vector3f* pointmax,
    Vector4f* rgbamin,
    Vector4f* rgbamax) const
{
    Vector3f point;
    Vector4f color(1.0f, 1.0f, 1.0f, 1.0f);

    // Default record size = 7 doubles (X,Y,Z,R,G,B,A)
    int numFields = 7;
    size_t recordSize = sizeof(T)* numFields;

    FILE* fin = fopen(filename.c_str(), "rb");

    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    size_t endpos = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    size_t numRecords = endpos / recordSize;
    //size_t readStart = numRecords * readStartP / BINARY_POINTS_MAX_BATCHES;
    //size_t readLength = numRecords * readLengthP / BINARY_POINTS_MAX_BATCHES;

    if(decimation <= 0) decimation = 1;
    if(readStart != 0)
    {
        fseek(fin, (long)(readStart * recordSize), SEEK_SET);
    }

    // Adjust read length.
    if(readLength == 0 || readStart + readLength > numRecords)
    {
        readLength = numRecords - readStart;
    }

    //ofmsg("BinaryPointsLoader: reading records %1% - %2% of %3% (decimation %4%) of %5%",
    //    %readStart % (readStart + readLength) % numRecords %decimation %filename);

    // Read in data
    T* buffer = (T*)malloc(recordSize * readLength / decimation);
    if(buffer == NULL)
    {
        oferror("BinaryPointsLoader::readXYZ: could not allocate %1% bytes",
            % (recordSize * readLength / decimation));
        return;
    }

    size_t ne = readLength / decimation;
    srand(100);
    // Read data
    // If data is not decimated, read it in one go.
    if(decimation == 1)
    {
        size_t size = fread(buffer, recordSize, readLength, fin);
    }
    else
    {
        int j = 0;
        for(int i = 0; i < ne; i++)
        {
            // // Read one record
            // size_t size = fread(&buffer[j], recordSize, 1, fin);
            // // Skip ahead decimation - 1 records.
            // fseek(fin, recordSize * (decimation - 1), SEEK_CUR);

            // RANDOM DECIMATED READ
            size_t recordoffset = rand() / (RAND_MAX / decimation + 1);
            size_t offs = ((size_t)recordSize) * (i * (decimation)+recordoffset);
            fseek(fin, (long)((readStart * recordSize) + offs), SEEK_SET);
            size_t size = fread(&buffer[j], recordSize, 1, fin);

            j += numFields;
        }
    }

    points->reserve(ne);
    colors->reserve(ne);

    size_t j = 0;
    for(size_t i = 0; i < ne; i++)
    {
        point[0] = buffer[i * numFields];
        point[1] = buffer[i * numFields + 1];
        point[2] = buffer[i * numFields + 2];

        color[0] = buffer[i * numFields + 3];
        color[1] = buffer[i * numFields + 4];
        color[2] = buffer[i * numFields + 5];
        color[3] = buffer[i * numFields + 6];

        points->push_back(point);
        colors->push_back(color);

        // Update data bounds and number of points
        *numPoints = *numPoints + 1;
        for(size_t j = 0; j < 4; j++)
        {
            if(color[j] < (*rgbamin)[j]) (*rgbamin)[j] = color[j];
            if(color[j] > (*rgbamax)[j]) (*rgbamax)[j] = color[j];
        }
        for(size_t j = 0; j < 3; j++)
        {
            if(point[j] < (*pointmin)[j]) (*pointmin)[j] = point[j];
            if(point[j] > (*pointmax)[j]) (*pointmax)[j] = point[j];
        }
    }

    fclose(fin);
    free(buffer);
}

///////////////////////////////////////////////////////////////////////////////
template<typename T>
T* readField(
    const String& filename,
    uint fieldIndex,
    size_t readStart, size_t readLength, int decimation,
    double* fmin,
    double* fmax)
{
    Vector3f point;
    Vector4f color(1.0f, 1.0f, 1.0f, 1.0f);

    // Default record size = 7 doubles (X,Y,Z,R,G,B,A)
    int numFields = 7;
    size_t recordSize = sizeof(T)* numFields;

    FILE* fin = fopen(filename.c_str(), "rb");

    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    size_t endpos = ftell(fin);
    fseek(fin, 0, SEEK_SET);
    size_t numRecords = endpos / recordSize;

    if(decimation <= 0) decimation = 1;
    if(readStart != 0)
    {
        fseek(fin, (long)(readStart * recordSize), SEEK_SET);
    }

    // Adjust read length.
    if(readLength == 0 || readStart + readLength > numRecords)
    {
        readLength = numRecords - readStart;
    }

    //ofmsg("BinaryPointsLoader: reading records %1% - %2% of %3% (decimation %4%) of %5%",
    //    %readStart % (readStart + readLength) % numRecords %decimation %filename);

    // Read in data
    T* buffer = (T*)malloc(recordSize * readLength / decimation);
    if(buffer == NULL)
    {
        oferror("BinaryPointsLoader::readXYZ: could not allocate %1% bytes",
            % (recordSize * readLength / decimation));
        return NULL;
    }

    size_t ne = readLength / decimation;
    srand(100);
    // Read data
    // If data is not decimated, read it in one go.
    if(decimation == 1)
    {
        size_t size = fread(buffer, recordSize, readLength, fin);
    }
    else
    {
        int j = 0;
        for(int i = 0; i < ne; i++)
        {
            // // Read one record
            // size_t size = fread(&buffer[j], recordSize, 1, fin);
            // // Skip ahead decimation - 1 records.
            // fseek(fin, recordSize * (decimation - 1), SEEK_CUR);

            // RANDOM DECIMATED READ
            size_t recordoffset = rand() / (RAND_MAX / decimation + 1);
            size_t offs = ((size_t)recordSize) * (i * (decimation)+recordoffset);
            fseek(fin, (long)((readStart * recordSize) + offs), SEEK_SET);
            size_t size = fread(&buffer[j], recordSize, 1, fin);

            j += numFields;
        }
    }

    size_t j = 0;
    for(size_t i = 0; i < ne; i++)
    {
        buffer[i] = buffer[i * numFields + fieldIndex];

        // Update data bounds and number of points
        *fmin = *fmin < buffer[i] ? *fmin : buffer[i];
        *fmax = *fmax > buffer[i] ? *fmax : buffer[i];
    }

    fclose(fin);
    
    return buffer;
}


///////////////////////////////////////////////////////////////////////////////
class LoadTask : public WorkerTask
{
public:
    Ref<Field> field;
    String path;

    void execute(WorkerTask::TaskInfo* ti)
    {
        String fullpath;
        if(DataManager::findFile(path, fullpath))
        {
            // Parse csv data column into a float array.
            int nrows = 0;
            double fmin = field->getDimension()->floatRangeMin;
            double fmax = field->getDimension()->floatRangeMax;
            int index = field->getDimension()->index;

            void* data = NULL;

            if(Dataset::useDoublePrecision())
            {
                data = readField<double>(fullpath, index, field->domain.start, field->domain.length, field->domain.decimation, &fmin, &fmax);
            }
            else
            {
                data = readField<float>(fullpath, index, field->domain.start, field->domain.length, field->domain.decimation, &fmin, &fmax);
            }

            field->getDimension()->floatRangeMin = fmin;
            field->getDimension()->floatRangeMax = fmax;

            field->lock.lock();
            // Extend the field data memory and copy the new data into it.
            size_t elemSize = field->getDimension()->getElementSize();
            if(field->data == NULL)
            {
                field->data = (char*)data;
            }
            else
            {
                free(field->data);
                field->data = (char*)data;
            }

            // Update field length
            field->loaded = true;
            field->stamp = otimestamp();

            field->lock.unlock();

            //ofmsg("Loading %1% finished", %field->getName());
        }
    }
};

///////////////////////////////////////////////////////////////////////////////
BinaryLoader::BinaryLoader()
{
    myNumRecords = 0;
    myLoaderPool.start(4);
}

///////////////////////////////////////////////////////////////////////////////
BinaryLoader::~BinaryLoader()
{
    myLoaderPool.stop();

}

///////////////////////////////////////////////////////////////////////////////
void BinaryLoader::open(const String& source)
{
    myFilename = source;
}

///////////////////////////////////////////////////////////////////////////////
void BinaryLoader::load(Field* f)
{
    LoadTask* task = new LoadTask();
    task->field = f;
    task->path = myFilename;
    myLoaderPool.queue(task);
}

///////////////////////////////////////////////////////////////////////////////
size_t BinaryLoader::getNumRecords()
{
    if(myNumRecords != 0) return myNumRecords;

    // Compute max records (points) from source file
    String path;
    if(!DataManager::findFile(myFilename, path))
    {
        ofwarn("BinaryLoader::getNumRecords: could not find %1%", %myFilename);
        return 0;
    }

    // Default record size = 7 doubles (X,Y,Z,R,G,B,A)
    size_t fs = Dataset::useDoublePrecision() ? sizeof(double) : sizeof(float);
    int numFields = 7;
    size_t recordSize = fs * numFields;
    FILE* fin = fopen(path.c_str(), "rb");
    // How many records are in the file?
    fseek(fin, 0, SEEK_END);
    size_t endpos = ftell(fin);
    size_t numRecords = endpos / recordSize;
    fclose(fin);

    myNumRecords = numRecords;
    return numRecords;
}

///////////////////////////////////////////////////////////////////////////////
int BinaryLoader::getDimensions()
{ 
    return 7; 
}

///////////////////////////////////////////////////////////////////////////////
bool BinaryLoader::getBounds(const Domain& d, float* bounds)
{
    String boundsBasename;
    String boundsExtension;
    String boundsPath;
    StringUtils::splitFullFilename(myFilename, boundsBasename, boundsExtension, boundsPath);
    String boundsFileName = ostr("%1%bounds/%2%.%3%-%4%-%5%.bounds", %boundsPath %boundsBasename %d.start %d.length %d.decimation);

    size_t length = d.length;
    size_t start = d.start;
    int decimation = d.decimation;

    // Try to read bounds from bounds cache
    if(readBoundsFile(boundsFileName, bounds)) return true;

    String path;
    if(DataManager::findFile(myFilename, path)) {
        Vector<Vector3f> points;
        Vector<Vector4f> colors;
        size_t numPoints = 0;
        float maxf = numeric_limits<float>::max();
        float minf = -numeric_limits<float>::max();
        Vector4f rgbamin = Vector4f(maxf, maxf, maxf, maxf);
        Vector4f rgbamax = Vector4f(minf, minf, minf, minf);
        Vector3f pointmin = Vector3f(maxf, maxf, maxf);
        Vector3f pointmax = Vector3f(minf, minf, minf);
        if(Dataset::useDoublePrecision()) readXYZ<double>(path, start, length, decimation, &points, &colors, &numPoints, &pointmin, &pointmax, &rgbamin, &rgbamax);
        else readXYZ<float>(path, start, length, decimation, &points, &colors, &numPoints, &pointmin, &pointmax, &rgbamin, &rgbamax);

        // make sure path exists
        String p = boundsPath + "bounds";
        //ofmsg("Creating path %1%", %p);
        DataManager::createPath(p);

        FILE* bf = fopen(boundsFileName.c_str(), "w");
        if(bf == NULL)
        {
            ofwarn("[BinaryLoader::getBounds] failed opening <%1%> for writing", %boundsFileName);
            return false;
        }
        fprintf(bf, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
            pointmin[0], pointmax[0],
            pointmin[1], pointmax[1],
            pointmin[2], pointmax[2],
            rgbamin[0], rgbamax[0],
            rgbamin[1], rgbamax[1],
            rgbamin[2], rgbamax[2],
            rgbamin[3], rgbamax[3]);
        fclose(bf);

        bounds[0] = pointmin[0]; bounds[1] = pointmax[0];
        bounds[2] = pointmin[1]; bounds[3] = pointmax[1];
        bounds[4] = pointmin[2]; bounds[5] = pointmax[2];
        bounds[6] = rgbamin[0]; bounds[7] = rgbamax[0];
        bounds[8] = rgbamin[1]; bounds[9] = rgbamax[1];
        bounds[10] = rgbamin[2]; bounds[11] = rgbamax[2];
        bounds[12] = rgbamin[3]; bounds[13] = rgbamax[3];

        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
bool BinaryLoader::readBoundsFile(const String& filename, float* bounds)
{
    String path;
    if(DataManager::findFile(filename, path))
    {
        String boundsText = DataManager::readTextFile(path);
        // Bounds text format:
        // xmin, xmax, ymin, ymax, zmin, zmax, rmin, rmax, gmin, gmax, bmin, bmax, amin, amax
        Vector<String> vals = StringUtils::split(boundsText, ",");
        for(int i = 0; i < 14; i++)
        {
            StringUtils::trim(vals[i]);
            bounds[i] = boost::lexical_cast<float>(vals[i]);
        }
        return true;
    }
    else
    {
        ofmsg("BinaryPointsReader::readBoundsFile bounds file not found %1% ", %filename);
    }
    return false;
}
