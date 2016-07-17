request_dependency(porthole)
request_dependency(hdf5)
module_version(2.0)

find_package(HDF5 REQUIRED COMPONENTS C HL)
include_directories(${HDF5_INCLUDE_DIRS})
add_library(signac MODULE 
    signac.cpp
    signac.h
    BinaryLoader.cpp
    BinaryLoader.h
    CsvLoader.cpp
    CsvLoader.h
    Dataset.cpp
    Dataset.h
    FireLoader.cpp
    FireLoader.h
    Hdf5Loader.cpp
    Hdf5Loader.h
    Loader.h
    PointCloud.cpp
    PointCloud.h
    Program.cpp
    Program.h
    Scatterplot.cpp
    Scatterplot.h)

target_link_libraries(signac omega ${HDF5_C_LIBRARIES} ${HDF5_HL_LIBRARIES})

declare_native_module(signac)
