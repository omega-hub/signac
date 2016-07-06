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
    Loader.h
    PointCloud.cpp
    PointCloud.h
    Program.cpp
    Program.h
    Scatterplot.cpp
    Scatterplot.h)
    
target_link_libraries(signac omega)

declare_native_module(signac)
