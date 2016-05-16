request_dependency(porthole)
module_version(2.0)

add_library(signac MODULE 
    signac.cpp
    CsvLoader.cpp
    CsvLoader.h
    Dataset.cpp
    Dataset.h
    Loader.h
    Scatterplot.cpp
    Scatterplot.h
    XyzbLoader.cpp)
    
target_link_libraries(signac omega)

declare_native_module(signac)
