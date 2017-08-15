set(LoggerLibSOURCES
        Logger/Logger.cpp)
add_library(LoggerLib SHARED ${LoggerLibSOURCES})

set(PingSOURCES
        Ping/ping.cpp)
add_library(PingLib SHARED ${PingSOURCES})

set(GatewaySOURCES
        MessageGateway/GatewayBase.cpp)
add_library(GatewayLib SHARED ${GatewaySOURCES})
target_link_libraries(GatewayLib PingLib)