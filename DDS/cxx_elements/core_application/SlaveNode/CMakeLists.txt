add_subdirectory(Worker)

set(SlaveGatewaySOURCES
        Slave/SlaveGateway.cpp)
add_library(SlaveGatewayLib SHARED ${SlaveGatewaySOURCES})
target_link_libraries(SlaveGatewayLib GatewayLib)

set(SlaveMessageHelperSOURCES
        Slave/SlaveMessageHelper.cpp)
add_library(SlaveMessageHelperLib SHARED ${SlaveMessageHelperSOURCES})
target_link_libraries(SlaveMessageHelperLib GatewayLib)

set(SlaveNodeObjectSOURCES
        Slave/Slave.cpp
        Slave/SlaveExceptions.hpp)
add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})
target_link_libraries(SlaveNodeObject LoggerLib DecryptorLib SlaveGatewayLib SlaveMessageHelperLib)