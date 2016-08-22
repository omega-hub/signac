request_dependency(porthole)
request_dependency(hdf5)
module_version(2.0)

include_directories(${CMAKE_BINARY_DIR}/3rdparty/hdf5/install/include)
if(OMEGA_OS_WIN)
    set(HDF5_C_LIBRARIES
        ${CMAKE_BINARY_DIR}/3rdparty/hdf5/install/lib/hdf5.lib
        ${CMAKE_BINARY_DIR}/3rdparty/hdf5/install/lib/hdf5_hl.lib)
else()
    set(HDF5_C_LIBRARIES
        libhdf5.dylib
        libhdf5_hl.dylib)
endif()

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

target_link_libraries(signac omega ${HDF5_C_LIBRARIES})

declare_native_module(signac)
