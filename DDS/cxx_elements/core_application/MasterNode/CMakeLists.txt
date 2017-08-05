set(WatchdogSOURCES
        Watchdog/Watchdog.cpp)

add_library(WatchdogLib SHARED ${WatchdogSOURCES})

set(MasterNodeObjectSOURCES
        Master/Master.cpp
        Master/MasterMessageGateway.cpp)

add_library(MasterNodeObject SHARED ${MasterNodeObjectSOURCES})

target_link_libraries(MasterNodeObject GatewayLib LoggerLib WatchdogLib)