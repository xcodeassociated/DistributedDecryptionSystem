set(MasterNodeObjectSOURCES
        Master/Master.cpp)

add_library(MasterNodeObject SHARED ${MasterNodeObjectSOURCES})
target_link_libraries(MasterNodeObject GatewayLib LoggerLib JsonFileOperationsLib)