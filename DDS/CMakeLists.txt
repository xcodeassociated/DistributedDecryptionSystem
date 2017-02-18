cmake_minimum_required(VERSION 3.5.1)
project(DDS)

## cmake options
option(ARM "Build for armhf" OFF)

option(O "Optimalize" 0)

option(CP "copy exec to given location" "")

if (ARM)
    ## armhf only configuration
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
    set(CROSS_COMPILER_PATH /usr/bin/)
    set(TOOLCHAIN_PREFIX ${CROSS_COMPILER_PATH}arm-linux-gnueabihf)
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

else()
    ## amd64 only configuration

endif()

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(APP_CORE cxx_elements/core_application)
set(CXX cxx_elements)


if (O STREQUAL "0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

elseif(O STREQUAL "1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")

elseif(O STREQUAL "2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

elseif(O STREQUAL "3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

endif()

## includes for main.cpp
include_directories(${APP_CORE}/controller)
include_directories(${APP_CORE}/model)
include_directories(${APP_CORE}/view)
include_directories(${CXX}/utilities)

set(MOCKS_DIR cxx_tests/test_source/mocks_source)

add_subdirectory(${CXX})

add_subdirectory(cxx_tests)

set(SOURCE_FILES ${CXX}/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} mvc)


## options checks
if (NOT CP STREQUAL "")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CP})

endif()
