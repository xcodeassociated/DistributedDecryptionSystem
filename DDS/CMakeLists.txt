cmake_minimum_required(VERSION 3.5.1)
project(DDS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Current DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message("Linux")

    set (CMAKE_PREFIX_PATH "/usr/include")
    set (CMAKE_PREFIX_PATH "/usr/include/mpi")
    set (CMAKE_PREFIX_PATH "/usr/include/openmpi")
    set (CMAKE_PREFIX_PATH "/usr/include/boost")

    set (CMAKE_PREFIX_PATH "/usr/lib/")
    set (CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/")

    include_directories("/usr/include/mpi")
    include_directories(${MPI_INCLUDE_PATH})
    include_directories(${Boost_INCLUDE_DIRS})

    find_package(MPI REQUIRED)
    if(MPI_FOUND)
        message(STATUS "MPI Found!")
        message(STATUS "MPI include: ${MPI_INCLUDE_PATH}")
    endif()

    find_package(Boost 1.5.8 COMPONENTS mpi serialization REQUIRED)
    if(Boost_FOUND)
        message(STATUS "Boost Found!")
        message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath -Wl,/usr/lib/openmpi/lib -Wl,--enable-new-dtags")

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

set(APP_CORE cxx_elements/core_application)
set(CXX cxx_elements)

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

target_link_libraries(${PROJECT_NAME}
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        pthread
        mvc)

## options checks
if (NOT CP STREQUAL "")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CP})

endif()
