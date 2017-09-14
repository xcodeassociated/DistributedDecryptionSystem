set(LoggerLibSOURCES
        Logger/Logger.cpp)
add_library(LoggerLib SHARED ${LoggerLibSOURCES})
target_link_libraries(LoggerLib ${Boost_LIBRARIES})

set(GatewaySOURCES
        MessageGateway/Gateway.cpp)
add_library(GatewayLib SHARED ${GatewaySOURCES})
target_link_libraries(GatewayLib ${MPI_LIBRARIES} ${Boost_LIBRARIES})

set(RunOptionsSOURCES
        RunOptions/RunOptions.cpp
        RunOptions/RunOptionsExceptions.hpp)
add_library(RunOptionsLib SHARED ${RunOptionsSOURCES})
target_link_libraries(RunOptionsLib ${Boost_LIBRARIES})

set(FileSOURCES
        File/FileCheck.cpp
        File/FileOperation.cpp
        File/FileExceptions.hpp)
add_library(FileLib SHARED ${FileSOURCES})
target_link_libraries(FileLib ${Boost_LIBRARIES})
