cmake_minimum_required(VERSION 3.6)
project(slave_node)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(SOURCE_FILES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

option(ARM "Build for armhf" OFF)

if (ARM)
    ## armhf only configuration

    set(TOOLCHAIN_PREFIX arm-linux-gnueabihf)
    SET(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
    SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)
    ##TODO: static or dynamic option needs to be added
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

else()
    ## amd64 only configuration

endif()



##############################################################

## Additional Targets:
#add_custom_target(${PROJECT_NAME}_armhf
#        $(MAKE) <options...>
#        # DEPENDS ${PROJECT_NAME} #
#        COMMENT "ARM ${PROJECT_NAME}")

#set_target_properties("${PROJECT_NAME} PROPERTIES <...>")

#execute_process(COMMAND ls OUTPUT_VARIABLE output)
#message(${output})