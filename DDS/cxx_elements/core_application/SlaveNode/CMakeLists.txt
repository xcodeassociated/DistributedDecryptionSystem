add_subdirectory(Worker)

set(SlaveNodeObjectSOURCES
        Slave/Slave.cpp
        Slave/SlaveMessageGateway.cpp)

add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})

target_link_libraries(SlaveNodeObject GatewayLib LoggerLib DecryptorLib)