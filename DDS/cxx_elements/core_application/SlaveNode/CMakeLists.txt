set(SlaveNodeObjectSOURCES
        MainClass/SlaveMainClass.hpp
        MainClass/SlaveMainClass.cpp)

## keyword STATIC for static lib linking
add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})
