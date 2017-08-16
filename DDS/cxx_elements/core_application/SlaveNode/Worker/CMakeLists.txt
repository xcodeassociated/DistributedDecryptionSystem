set(WorkerBaseLibSOURCES
        WorkerBase/WorkerBase.cpp)

set(DecryptorLibSOURCES
        Decryptor/Decryptor.cpp)

add_library(DecryptorLib SHARED ${WorkerBaseLibSOURCES} ${DecryptorLibSOURCES})
target_link_libraries(DecryptorLib ${Boost_LIBRARIES} ${CRYPTOPP_LIBRARY})
