add_subdirectory(Worker)

set(SlaveNodeObjectSOURCES
        Slave/Slave.cpp)

add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})
target_link_libraries(SlaveNodeObject GatewayLib LoggerLib DecryptorLib)