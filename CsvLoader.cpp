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

    void execute(WorkerTask::TaskInfo* ti)
    {
        String fullpath;
        if(DataManager::findFile(path, fullpath))
        {
            // Read CSV block.
            FILE* f = fopen(fullpath.c_str(), "rb");
            fseek(f, blockStart, SEEK_SET);
            char* csv = (char*)malloc(blockLength);
            size_t readSize = fread(csv, 1, blockLength, f);

            // Are we reading the end of the file?
            bool final = readSize < blockLength;

            // Parse csv data column into a float array.
            int nrows;
            float min = field->getInfo()->floatRangeMin;
            float max = field->getInfo()->floatRangeMax;

            float* data = parseFloatField(csv, readSize, field->getInfo()->index, &nrows, &min, &max);

            field->getInfo()->floatRangeMin = min;
            field->getInfo()->floatRangeMax = max;

            field->lock.lock();
            // Extend the field data memory and copy the new data into it.
            if(field->data == NULL)
            {
                field->data = (char*)malloc(nrows * sizeof(float));
            }
            else
            {
                field->data = (char*)realloc(field->data, (field->length + nrows) * sizeof(float));
            }
            memcpy(
                &field->data[field->length * field->getInfo()->getElementSize()],
                data,
                nrows * sizeof(float));

            // Update field length
            field->length += nrows;

            field->lock.unlock();

            free(data);
            free(csv);
            fclose(f);

            field->loaded = final;
            field->stamp = otimestamp();

            if(final)
            {
                ofmsg("Loading %1% finished", %field->getInfo()->id);
            }
            else
            {
                blockStart += BLOCK_SIZE;
                getPool()->queue(this);
            }
        }
    }

    // parse a float column from csv string data. return the array of parsed floats
    // and the number of parsed rows.
    float* parseFloatField(char* csv, uint csvsize, int fieldidx, int* nrows, float* vmin, float* vmax)
    {
        *nrows = 0;
        float* data = NULL;
        char* rows[MAX_ROWS_PER_BLOCK];

        // count rows and save beginning of each row in rows array
        rows[0] = csv;
        for(uint i = 0; i < csvsize; i++) if(csv[i] == '\n')
        {
            csv[i] = 0;
            (*nrows)++;
            rows[*nrows] = &csv[i + 1];
        }

        data = (float*)malloc(*nrows * sizeof(float));

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
            data[i - 1] = atof(fieldstart);
            *vmin = std::min(*vmin, data[i - 1]);
            *vmax = std::max(*vmax, data[i - 1]);
        }

        return data;
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
