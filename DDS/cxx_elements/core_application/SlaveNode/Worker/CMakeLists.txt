include_directories(WorkerBase)

set(WorkerBaseLibSOURCES
        WorkerBase/WorkerBase.hpp
        WorkerBase/WorkerBase.cpp)

set(DecryptorLibSOURCES
        Decryptor/Decryptor.hpp
        Decryptor/Decryptor.cpp)

add_library(WorkerBaseLib SHARED ${WorkerBaseLibSOURCES})

add_library(DecryptorLib SHARED ${DecryptorLibSOURCES})

target_link_libraries(DecryptorLib WorkerBaseLib)