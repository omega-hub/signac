#include <hdf5.h>

#include "FireLoader.h"

///////////////////////////////////////////////////////////////////////////////
size_t FireLoader::getNumRecords(Dataset* d)
{
    hid_t file_id;
    String fullpath;
    if(DataManager::findFile(myFilename, fullpath))
    {
        file_id = H5Fopen(fullpath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
        hid_t header_id = H5Gopen2(file_id, "/Header", H5P_DEFAULT);
        hid_t numpart_id = H5Aopen_by_name(header_id, ".", "NumPart_Total", H5P_DEFAULT, H5P_DEFAULT);

        String dname = d->getName();
        int did = boost::lexical_cast<int>(dname.at(dname.length() - 1));

        int npa[6];
        H5Aread(numpart_id, H5T_NATIVE_INT32, npa);

        H5Aclose(numpart_id);
        H5Gclose(header_id);
        H5Fclose(file_id);

        return npa[did];
    }
    return 0;
}
