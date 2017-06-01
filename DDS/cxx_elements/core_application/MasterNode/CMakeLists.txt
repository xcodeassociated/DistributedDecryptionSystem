include_directories(Watchdog)

set(WatchdogSOURCES
        Watchdog/Watchdog.hpp
        Watchdog/Watchdog.cpp)

add_library(WatchdogLib SHARED ${WatchdogSOURCES})

set(MasterNodeObjectSOURCES
        Master/Master.hpp
        Master/Master.cpp)

add_library(MasterNodeObject SHARED ${MasterNodeObjectSOURCES})

target_link_libraries(MasterNodeObject WatchdogLib)