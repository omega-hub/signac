#include <hdf5.h>

#include "signac.h"
#include "Hdf5Loader.h"

// Maximum number of rows that can be read for each loaded block.
#define MAX_ROWS_PER_BLOCK 65535
#define BLOCK_SIZE 4096000 

// Lock to serialize HDF5 operations
Lock hdf5APIlock;

///////////////////////////////////////////////////////////////////////////////
class Hdf5LoadTask : public WorkerTask
{
public:
    Ref<Field> field;
    String path;
    uint blockStart;
    uint blockLength;
    char** rows;

    Hdf5LoadTask()
    {
    }

    ~Hdf5LoadTask()
    {
    }

    void execute(WorkerTask::TaskInfo* ti)
    {
        hid_t file_id;
        herr_t status;
        String fullpath;
        if(DataManager::findFile(path, fullpath))
        {
            hdf5APIlock.lock();
            file_id = H5Fopen(fullpath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

            // Find the dimension and datset name
            String dimname = field->getDimension()->id;
            String dsetname = ostr("/%1%/%2%", 
                %field->getDimension()->dataset->getName()
                %dimname);
            int colidx = field->getDimension()->index;
            
            // If the domain has a stream id, use the stream offset as the read
            // start, instead of the domain start. This is to support multipart
            // files.
            size_t sstart = field->domain.start;
            if(field->domain.streamid != -1) sstart = field->domain.streamoffset;
            size_t slen = field->domain.length;
            int sstride = field->domain.decimation;


            hid_t dataset_id = H5Dopen2(file_id, dsetname.c_str(), H5P_DEFAULT);
            if(dataset_id == -1)
            {
                ofwarn("Failed opening dataset %1%", %dsetname);
                return;
            }
            hid_t dspace_id = H5Dget_space(dataset_id);

            size_t nr = field->getDimension()->dataset->getNumRecords();
            if(sstart + slen > nr)
            {
                slen = nr - sstart;
                field->domain.length = slen;
            }

            slen = slen / sstride;

            // Setup selection
            hsize_t dims[2], start[2], stride[2], count[2];
            dims[0] = slen;
            start[1] = colidx; start[0] = sstart;
            count[0] = slen; count[1] = 1;
            stride[0] = sstride; stride[1] = 1;

            status = H5Sselect_hyperslab(dspace_id, H5S_SELECT_SET, start, stride, count, NULL);
            oassert(status != -1);

            hid_t mspace_id = H5Screate_simple(1, dims, NULL);

            int p1 = H5Sget_select_npoints(dspace_id);
            int p2 = H5Sget_select_npoints(mspace_id);
            oassert(p1 == p2);

            oflog(Debug, "reading %1% - offs %2%", %dsetname %sstart);
            float* fielddata = (float*)malloc(p1 * sizeof(float));
            status = H5Dread(dataset_id, H5T_IEEE_F32LE, mspace_id, dspace_id, H5P_DEFAULT, fielddata);
            oassert(status != -1);
            hdf5APIlock.unlock();

            field->lock.lock();
            // Update dimension bounds
            float fmin = field->getDimension()->floatRangeMin;
            float fmax = field->getDimension()->floatRangeMax;
            for(int i = 0; i < p1; i++)
            {
                fmin = fmin < fielddata[i] ? fmin : fielddata[i];
                fmax = fmax > fielddata[i] ? fmax : fielddata[i];
                field->boundMin = field->boundMin < fielddata[i] ? field->boundMin : fielddata[i];
                field->boundMax = field->boundMax > fielddata[i] ? field->boundMax : fielddata[i];
            }
            field->getDimension()->floatRangeMin = fmin;
            field->getDimension()->floatRangeMax = fmax;
            // Update field length
            field->data = (char*)fielddata;
            field->loaded = true;
            field->stamp = otimestamp();
            field->lock.unlock();

            Signac::instance->signalFieldLoaded(field);

            hdf5APIlock.lock();
            H5Sclose(mspace_id);
            H5Sclose(dspace_id);
            H5Dclose(dataset_id);
            H5Fclose(file_id);
            hdf5APIlock.unlock();
        }
    }

};

///////////////////////////////////////////////////////////////////////////////
Hdf5Loader::~Hdf5Loader()
{
}

///////////////////////////////////////////////////////////////////////////////
void Hdf5Loader::open(const String& source)
{
    if(!DataManager::findFile(source, myFilename))
    {
        ofwarn("[Hdf5Loader::open] could not find %1%", %source);
    }
}

///////////////////////////////////////////////////////////////////////////////
size_t Hdf5Loader::getNumRecords(Dataset* d)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
void Hdf5Loader::load(Field* f)
{
    if(f->getDimension()->dataset->useDoublePrecision())
    {
        oerror("[Hdf5Loader] double precision reads not supported yet.");
    }

    Hdf5LoadTask* task = new Hdf5LoadTask();
    task->field = f;
    task->blockStart = 0;
    task->blockLength = BLOCK_SIZE;
    task->path = myFilename;

    Signac::instance->addTask(task);
}
