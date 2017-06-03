add_subdirectory(Worker)

set(SlaveNodeObjectSOURCES
        Slave/Slave.hpp
        Slave/Slave.cpp)

add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})

target_link_libraries(SlaveNodeObject LoggerLib DecryptorLib)