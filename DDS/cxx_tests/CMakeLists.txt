project(DDS_UnitTests)

add_definitions(-DTESTING=1)

add_subdirectory(test_libs)
enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

set(APP_CORE ${CMAKE_SOURCE_DIR}/cxx_elements/core_application)
set(COMMON ${CMAKE_SOURCE_DIR}/cxx_elements/common)

include_directories(${APP_CORE}/SlaveNode/Worker/WorkerBase)
include_directories(${APP_CORE}/SlaveNode/Worker/Decryptor)

set(MOCK test_mocked_headres)

include_directories(${MOCK})

set(TEST_Logger
        ${COMMON}/Logger/Logger.cpp)
add_library(TEST_LoggerLib SHARED ${TEST_Logger})
target_link_libraries(TEST_LoggerLib ${Boost_LIBRARIES})

set(TEST_Ping
        ${COMMON}/Ping/ping.cpp)
add_library(TEST_PingLib SHARED ${TEST_Ping})
target_link_libraries(TEST_PingLib ${Boost_LIBRARIES})

set(TEST_Gateway
        ${COMMON}/MessageGateway/Gateway.cpp)
add_library(TEST_GatewayLib SHARED ${TEST_Gateway})
target_link_libraries(TEST_GatewayLib TEST_PingLib ${MPI_LIBRARIES} ${Boost_LIBRARIES})

set(TEST_WorkerBaseLibSOURCES
        ${APP_CORE}/SlaveNode/Worker/WorkerBase/WorkerBase.cpp)
set(TEST_DecryptorLibSOURCES
        ${APP_CORE}/SlaveNode/Worker/Decryptor/Decryptor.cpp)
add_library(TEST_DecryptorLib SHARED ${TEST_WorkerBaseLibSOURCES} ${TEST_DecryptorLibSOURCES})
target_link_libraries(TEST_DecryptorLib ${CRYPTOPP_LIBRARY} ${Boost_LIBRARIES})

set(TEST_Master
        ${APP_CORE}/MasterNode/Master/Master.cpp)
add_library(TEST_MasterLib SHARED ${TEST_Master})
target_link_libraries(TEST_MasterLib TEST_LoggerLib TEST_GatewayLib)

set(TEST_Slave
        ${APP_CORE}/SlaveNode/Slave/Slave.hpp
        ${APP_CORE}/SlaveNode/Slave/Slave.cpp)
add_library(TEST_SlaveLib SHARED ${TEST_Slave})
target_link_libraries(TEST_SlaveLib TEST_LoggerLib TEST_GatewayLib TEST_DecryptorLib)

set(TEST_MASTER_SOURCES
        test_implementation/Master_test.cpp
        ${MOCK}/Master_mock.hpp
        ${MOCK}/Slave_mock.hpp)

set(TEST_SLAVE_SOURCES
        test_implementation/Slave_test.cpp
        ${MOCK}/Master_mock.hpp
        ${MOCK}/Slave_mock.hpp)

add_executable(${PROJECT_NAME}_Master
        ${GMOCK_TESTS_SOURCE_FILES}
        ${TEST_MASTER_SOURCES})

add_executable(${PROJECT_NAME}_Slave
        ${GMOCK_TESTS_SOURCE_FILES}
        ${TEST_SLAVE_SOURCES})

target_link_libraries(${PROJECT_NAME}_Master
        TEST_MasterLib
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${CRYPTOPP_LIBRARY}
        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

target_link_libraries(${PROJECT_NAME}_Slave
        TEST_SlaveLib
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${CRYPTOPP_LIBRARY}
        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

add_test(${PROJECT_NAME}_Master UnitTests)

add_test(${PROJECT_NAME}_Slave UnitTests)