set(LoggerLibSOURCES
        Logger/Logger.cpp)

add_library(LoggerLib SHARED ${LoggerLibSOURCES})

set(GatewaySOURCES
        Message/MPI/GatewayBase.cpp)

add_library(GatewayLib SHARED ${GatewaySOURCES})