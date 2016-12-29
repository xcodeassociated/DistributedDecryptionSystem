cmake_minimum_required(VERSION 3.6)
project(slave_node)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(APP_CORE cxx_elements/core_application)

include_directories(cxx_elements)
include_directories(${APP_CORE})

include_directories(${APP_CORE}/controller)
include_directories(${APP_CORE}/model)
include_directories(${APP_CORE}/view)

set(SOURCE_FILES main.cpp
        ${APP_CORE}/controller/Controller.cpp ${APP_CORE}/controller/Controller.hpp
        ${APP_CORE}/view/View.cpp ${APP_CORE}/view/View.hpp
        ${APP_CORE}/model/Model.cpp ${APP_CORE}/model/Model.hpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

#TODO: add custom bash commands
add_custom_command(
        OUTPUT FILENAME.txt
        #DEPENDS ${PROJECT_NAME}
        COMMAND ${CMAKE_COMMAND} -E touch FILENAME.txt
        COMMAND ${CMAKE_COMMAND} -E echo "creating FILENAME"
)

option(ARM "Build for armhf" OFF)

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