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

set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} mvc)

#TODO: add custom bash commands
add_custom_command(
        OUTPUT FILENAME.txt
        #DEPENDS ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E touch FILENAME.txt
        COMMAND ${CMAKE_COMMAND} -E echo "creating FILENAME"
)

## cmake options
option(ARM "Build for armhf" OFF)

option(O "Optimalize" 0)

## options checks
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
    ##TODO: static or dynamic option needs to be added
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

else()
    ## amd64 only configuration

endif()