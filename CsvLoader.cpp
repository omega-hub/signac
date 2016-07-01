#include "CsvLoader.h"

// Maximum number of rows that can be read for each loaded block.
#define MAX_ROWS_PER_BLOCK 65535
#define BLOCK_SIZE 4096000 

///////////////////////////////////////////////////////////////////////////////
class LoadTask : public WorkerTask
{
public:
    Ref<Field> field;
    String path;
    uint blockStart;
    uint blockLength;
    char** rows;
    
    LoadTask()
    {
        rows = (char**)malloc(MAX_ROWS_PER_BLOCK * sizeof(char*));
    }
    
    ~LoadTask()
    {
        free(rows);
    }

    // parse a float column from csv string data. return the array of parsed floats
    // and the number of parsed rows.
    template<typename T>
    void* parseFloatField(char* csv, size_t csvsize, int fieldidx, int* nrows, double* vmin, double* vmax)
    {
        *nrows = 0;
        T* data = NULL;

        // count rows and save beginning of each row in rows array
        rows[0] = csv;
        for(uint i = 0; i < csvsize; i++) if(csv[i] == '\n')
        {
            csv[i] = 0;
            (*nrows)++;
            rows[*nrows] = &csv[i + 1];
        }

        data = (T*)malloc(*nrows * sizeof(T));

        // Parse fieldidx-th column for each row.
        // HARDCODED SKIP HEADER
        for(uint i = 1; i < *nrows; i++)
        {
            int c = fieldidx;

            char* fieldstart = rows[i];
            while(c > 0)
            {
                fieldstart = strchr(fieldstart, ',');
                // skip the comma
                fieldstart++;
                c--;
            }
            T v = atof(fieldstart);
            data[i - 1] = v;
            *vmin = *vmin < v ? *vmin : v;
            *vmax = *vmax > v ? *vmax : v;
        }

        return data;
    }

    void execute(WorkerTask::TaskInfo* ti)
    {
        String fullpath;
        if(DataManager::findFile(path, fullpath))
        {
            // Read CSV block.
            FILE* f = fopen(fullpath.c_str(), "rb");
            
            if(f == NULL)
            {
                oferror("[signac:LoadTask] Could not open file %1%", %fullpath);
            }
            
            fseek(f, blockStart, SEEK_SET);
            char* csv = (char*)malloc(blockLength);
            oassert(csv != NULL);
            size_t readSize = fread(csv, 1, blockLength, f);

            // Are we reading the end of the file?
            bool final = readSize < blockLength;

            // Parse csv data column into a float array.
            int nrows = 0;
            double min = field->getDimension()->floatRangeMin;
            double max = field->getDimension()->floatRangeMax;
            int index = field->getDimension()->index;

            void* data = NULL;
            
            if(Dataset::useDoublePrecision())
            {
                data = parseFloatField<double>(csv, readSize, index, &nrows, &min, &max);
            }
            else
            {
                data = parseFloatField<float>(csv, readSize, index, &nrows, &min, &max);
            }

            field->getDimension()->floatRangeMin = min;
            field->getDimension()->floatRangeMax = max;

            field->lock.lock();
            // Extend the field data memory and copy the new data into it.
            size_t elemSize = field->getDimension()->getElementSize();
            if(field->data == NULL)
            {
                field->data = (char*)malloc(nrows * elemSize);
            }
            else
            {
                field->data = (char*)realloc(field->data, (field->domain.length + nrows) * elemSize);
            }
            memcpy(
                &field->data[field->domain.length * field->getDimension()->getElementSize()],
                data,
                nrows * elemSize);

            // Update field length
            field->domain.length += nrows;
            field->loaded = final;
            field->stamp = otimestamp();
            //ofmsg("Field %1% l=%2%", %field->getInfo()->id %field->length);

            field->lock.unlock();

            free(data);
            free(csv);
            fclose(f);

            if(final)
            {
                ofmsg("Loading %1% finished", %field->getDimension()->id);
            }
            else
            {
                blockStart += BLOCK_SIZE;
                //ofmsg("Queuing block %1%", %blockStart);
                getPool()->queue(this);
            }
        }
    }

};

///////////////////////////////////////////////////////////////////////////////
CsvLoader::~CsvLoader()
{
    myLoaderPool.stop();
}

///////////////////////////////////////////////////////////////////////////////
void CsvLoader::open(const String& source)
{
    myFilename = source;
    myLoaderPool.start(4);
}

///////////////////////////////////////////////////////////////////////////////
void CsvLoader::load(Field* f)
{
    LoadTask* task = new LoadTask();
    task->field = f;
    task->blockStart = 0;
    task->blockLength = BLOCK_SIZE;
    task->path = myFilename;
    myLoaderPool.queue(task);
}
