set(LoggerLibSOURCES
        Logger/Logger.cpp)
add_library(LoggerLib SHARED ${LoggerLibSOURCES})
target_link_libraries(LoggerLib ${Boost_LIBRARIES})

set(GatewaySOURCES
        MessageGateway/Gateway.cpp
        MessageGateway/GatewayExceptions.hpp)
add_library(GatewayLib SHARED ${GatewaySOURCES})
target_link_libraries(GatewayLib ${MPI_LIBRARIES} ${Boost_LIBRARIES})

set(RunOptionsSOURCES
        RunOptions/RunOptions.cpp
        RunOptions/RunOptionsExceptions.hpp)
add_library(RunOptionsLib SHARED ${RunOptionsSOURCES})
target_link_libraries(RunOptionsLib ${Boost_LIBRARIES})