include_directories(Worker/WorkerBase)
include_directories(Worker/Decryptor)

add_subdirectory(Worker)

set(SlaveNodeObjectSOURCES
        Slave/Slave.hpp
        Slave/Slave.cpp)

add_library(SlaveNodeObject SHARED ${SlaveNodeObjectSOURCES})

target_link_libraries(SlaveNodeObject DecryptorLib)