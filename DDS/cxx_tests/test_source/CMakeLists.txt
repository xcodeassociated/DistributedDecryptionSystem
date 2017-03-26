enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

set(MOCK mocks_headres)

include_directories(${MOCK})

set(TEST_SOURCES
        test_implementation/SlaveMainClass_test.cpp
        test_implementation/MasterMainClass_test.cpp
        ${MOCK}/MasterMainClass_mock.hpp
        ${MOCK}/SlaveMainClass_mock.hpp)

add_executable(RunTests
        ${GMOCK_TESTS_SOURCE_FILES}
        ${TEST_SOURCES})

target_link_libraries(RunTests
        gtest
        gtest_main
        gmock
        gmock_main
        pthread)

add_test(RunTests RunTests)