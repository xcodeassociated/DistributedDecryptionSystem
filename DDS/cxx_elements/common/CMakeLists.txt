set(LoggerLibSOURCES
        Logger/Logger.cpp)
add_library(LoggerLib SHARED ${LoggerLibSOURCES})
target_link_libraries(LoggerLib ${Boost_LIBRARIES})

set(PingSOURCES
        Ping/ping.cpp)
add_library(PingLib SHARED ${PingSOURCES})
target_link_libraries(PingLib ${Boost_LIBRARIES})

set(GatewaySOURCES
        MessageGateway/Gateway.cpp)
add_library(GatewayLib SHARED ${GatewaySOURCES})
target_link_libraries(GatewayLib PingLib ${MPI_LIBRARIES} ${Boost_LIBRARIES})

set(JsonFileOperationsSOURCES
        JsonFileOperations/JsonFileOperations.cpp)
add_library(JsonFileOperationsLib SHARED ${JsonFileOperationsSOURCES})