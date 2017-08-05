set(WorkerBaseLibSOURCES
        WorkerBase/WorkerBase.cpp)

set(DecryptorLibSOURCES
        Decryptor/Decryptor.cpp)

add_library(DecryptorLib SHARED ${WorkerBaseLibSOURCES} ${DecryptorLibSOURCES})
