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
        MasterNodeObject

        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

target_link_libraries(${PROJECT_NAME}_Slave
        SlaveNodeObject

        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

add_test(${PROJECT_NAME}_Master UnitTests)

add_test(${PROJECT_NAME}_Slave UnitTests)