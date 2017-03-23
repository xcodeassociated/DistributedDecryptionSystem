set(MasterNodeObjectSOURCES
        MainClass/MasterMainClass.hpp
        MainClass/MasterMainClass.cpp)

## keyword STATIC for static lib linking
add_library(MasterNodeObject SHARED ${MasterNodeObjectSOURCES})
