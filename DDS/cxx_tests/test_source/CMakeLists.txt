enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${gmock_SOURCE_DIR}/include ${gmock_SOURCE_DIR})

set(MOCK mocks_source)

include_directories(${MOCK})

set(TEST_SOURCES
        mvc_controller_tests.cpp
        mvc_model_tests.cpp
        ${MOCK}/ModelMock.hpp)

add_executable(RunTests
        ${GMOCK_TESTS_SOURCE_FILES}
        ${TEST_SOURCES})

target_link_libraries(RunTests
        gtest
        gtest_main
        gmock
        gmock_main
        pthread
        mvc)

add_test(RunTests RunTests)