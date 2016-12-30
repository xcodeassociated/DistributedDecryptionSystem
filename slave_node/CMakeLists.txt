cmake_minimum_required(VERSION 3.5.1)
project(slave_node)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(APP_CORE cxx_elements/core_application)

## includes for main.cpp
include_directories(${APP_CORE}/controller)
include_directories(${APP_CORE}/model)
include_directories(${APP_CORE}/view)

add_subdirectory(cxx_elements)

add_subdirectory(cxx_tests)

set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} mvc)

## cmake options
option(ARM "Build for armhf" OFF)

option(O "Optimalize" 0)

option(CP "copy exec to given location" "")

## options checks
if (NOT CP STREQUAL "")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CP})

endif()

if (O STREQUAL "0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

elseif(O STREQUAL "1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")

elseif(O STREQUAL "2")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

elseif(O STREQUAL "3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

endif()

if (ARM)
    ## armhf only configuration
    set(TOOLCHAIN_PREFIX arm-linux-gnueabihf)
    set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

else()
    ## amd64 only configuration

endif()