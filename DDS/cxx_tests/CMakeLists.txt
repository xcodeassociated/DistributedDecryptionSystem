project(DDS_UnitTests)

add_subdirectory(test_libs)
enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

set(MOCK test_mocked_headres)

include_directories(${MOCK})

set(TEST_SOURCES
        test_implementation/Slave_test.cpp
        test_implementation/Master_test.cpp
        ${MOCK}/Master_mock.hpp
        ${MOCK}/Slave_mock.hpp)

add_executable(${PROJECT_NAME}
        ${GMOCK_TESTS_SOURCE_FILES}
        ${TEST_SOURCES})

target_link_libraries(${PROJECT_NAME}
        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

add_test(${PROJECT_NAME} UnitTests)