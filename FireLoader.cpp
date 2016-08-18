#include <hdf5.h>

#include "FireLoader.h"

///////////////////////////////////////////////////////////////////////////////
size_t getHeaderArrayValue(const String& filename, const String& arrayname, int index)
{
    hid_t file_id;
    String fullpath;
    if(DataManager::findFile(filename, fullpath))
    {
        file_id = H5Fopen(fullpath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        hid_t header_id = H5Gopen2(file_id, "/Header", H5P_DEFAULT);
        hid_t numpart_id = H5Aopen_by_name(header_id, ".", arrayname.c_str(), H5P_DEFAULT, H5P_DEFAULT);

        int npa[6];
        H5Aread(numpart_id, H5T_NATIVE_INT32, npa);

        H5Aclose(numpart_id);
        H5Gclose(header_id);
        H5Fclose(file_id);

        return npa[index];
    }
}

///////////////////////////////////////////////////////////////////////////////
FireLoader::FireLoader() :
myRecordsPerFile(NULL)
{

}

///////////////////////////////////////////////////////////////////////////////
FireLoader::~FireLoader()
{
    delete myRecordsPerFile;
}

///////////////////////////////////////////////////////////////////////////////
size_t FireLoader::getNumRecords(Dataset* d)
{
    // Do we have the number of records cached for this dataset?
    if(myNumRecords.find(d) != myNumRecords.end()) return myNumRecords[d];

    String dname = d->getName();
    int did = boost::lexical_cast<int>(dname.at(dname.length() - 1));
    size_t nr = getHeaderArrayValue(mySnapshotPaths[0], "NumPart_Total", did);
    if(nr != 0) myNumRecords[d] = nr;
    return nr;
}

///////////////////////////////////////////////////////////////////////////////
void FireLoader::open(const String& source)
{
    mySnapshotPaths = StringUtils::split(source, ";");

    ofmsg("[FireLoader::open] setting up loader for <%1%> parts", %mySnapshotPaths.size());
    foreach(String& s, mySnapshotPaths)
    {
        StringUtils::trim(s);
        Hdf5Loader* l = new Hdf5Loader();
        l->open(s);
        myLoaders.push_back(l);
    }

    myRecordsPerFile = new size_t[MaxParts * mySnapshotPaths.size()];
    memset(myRecordsPerFile, 0, MaxParts * mySnapshotPaths.size() * sizeof(size_t));
}

///////////////////////////////////////////////////////////////////////////////
void FireLoader::load(Field* f)
{
    int numFiles = mySnapshotPaths.size();
    size_t start = f->domain.start;
    size_t length = f->domain.length;

    size_t fileStart = 0;
    size_t fileLength = 0;

    // Get the datapart id (last character of the dataset name, corresponding to
    // the HDF5 PartType[number] names
    String dsetname = f->getDimension()->dataset->getName();
    int did = boost::lexical_cast<int>(dsetname.at(dsetname.length() - 1));

    for(int curFile = 0; curFile < numFiles; curFile++)
    {
        // Get the number of records for this field's dimension in the current
        // file
        if(myRecordsPerFile[curFile * numFiles + did] == 0)
        {
            size_t r = getHeaderArrayValue(mySnapshotPaths[curFile], "NumPart_ThisFile", did);
            oflog(Debug, "[FireLoader::load] num records for <%1%> part <%2%>: <%3%>",
                %mySnapshotPaths[curFile] %did %r);
            myRecordsPerFile[curFile * numFiles + did] = r;
        }
        fileLength = myRecordsPerFile[curFile * numFiles + did];

        // Is the start of this field in this file?
        if(start > fileStart && start < fileStart + fileLength)
        {
            // Is the length of this field contained within this file?
            if(start + length < fileStart + fileLength)
            {
                f->domain.streamid = curFile;
                f->domain.streamoffset = start - fileStart;
                // HACK: if the stream offset is more than 0 but less than length,
                // this means a previous field for this dimension crossed file
                // boundaries. We adjust the start position of this field to be
                // the beginning of the current file to avoid gaps.
                // NOTE: this works only because we make a lot of assumptions
                // on how data is loaded (ie all fields have the same length)
                if(f->domain.streamoffset > 0 && f->domain.streamoffset < length)
                {
                    f->domain.streamoffset = 0;
                }
                myLoaders[curFile]->load(f);
                break;
            }
            else
            {
                // This field's domain crosses the file boundaries. Adjust.
                f->domain.streamid = curFile;
                f->domain.streamoffset = start - fileStart;
                f->domain.length = fileLength - f->domain.streamoffset;
            }
        }
        fileStart += fileLength;
    }
}
