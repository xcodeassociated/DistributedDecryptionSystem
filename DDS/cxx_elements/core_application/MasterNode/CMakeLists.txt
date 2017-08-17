set(MasterNodeObjectSOURCES
        Master/Master.cpp
        Master/MasterMessageHelper.cpp
        Master/MasterGateway.cpp)

add_library(MasterNodeObject SHARED ${MasterNodeObjectSOURCES})

set(MasterHelperSOURCES
        Master/MasterMessageHelper.cpp)
add_library(MasterHelperLib SHARED ${MasterHelperSOURCES})
target_link_libraries(MasterHelperLib GatewayLib)

set(MasterGatewaySOURCES
        Master/MasterGateway.cpp
        Master/MasterExceptions.hpp)
add_library(MasterGatewayLib SHARED ${MasterGatewaySOURCES})
target_link_libraries(MasterGatewayLib GatewayLib)

target_link_libraries(MasterNodeObject LoggerLib JsonFileOperationsLib MasterGatewayLib)
