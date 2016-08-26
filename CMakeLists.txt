request_dependency(porthole)
request_dependency(hdf5)
module_version(2.0)

add_library(signac MODULE 
    signac.cpp
    signac.h
    BinaryLoader.cpp
    BinaryLoader.h
    CsvLoader.cpp
    CsvLoader.h
    Dataset.cpp
    Dataset.h
    Filter.cpp
    Filter.h
    FireLoader.cpp
    FireLoader.h
    Hdf5Loader.cpp
    Hdf5Loader.h
    Loader.h
    PointBatch.cpp
    PointBatch.h
    PointCloud.cpp
    PointCloud.h
    PointCloudView.cpp
    PointCloudView.h
    Program.cpp
    Program.h
    Scatterplot.cpp
    Scatterplot.h)

target_link_libraries(signac omega hdf5)

declare_native_module(signac)
