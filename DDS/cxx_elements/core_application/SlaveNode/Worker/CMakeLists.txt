set(WorkerBaseLibSOURCES
        WorkerBase/WorkerBase.hpp
        WorkerBase/WorkerBase.cpp)

set(DecryptorLibSOURCES
        Decryptor/Decryptor.hpp
        Decryptor/Decryptor.cpp)

add_library(DecryptorLib SHARED ${WorkerBaseLibSOURCES} ${DecryptorLibSOURCES})
