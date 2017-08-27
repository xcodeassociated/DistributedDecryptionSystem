cmake_minimum_required(VERSION 3.5.1)
project(DDS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Source DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

option(O "Optimalize" 0)

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

message (STATUS "Build DIR: ${CMAKE_CURRENT_BINARY_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules")

find_package(MPI REQUIRED)
if(MPI_FOUND)
    include_directories(${MPI_INCLUDE_PATH})

    message(STATUS "MPI Found!")
    message(STATUS "MPI include: ${MPI_INCLUDE_PATH}")
    message(STATUS "MPI libs: ${MPI_LIBRARIES}")
endif()

find_package(Boost COMPONENTS mpi serialization program_options chrono thread system filesystem regex REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})

    message(STATUS "Boost Found!")
    message(STATUS "Boost version: ${Boost_VERSION}")
    message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libs: ${Boost_LIBRARY_DIRS}")
endif()

find_package(CryptoPP REQUIRED)
if (CryptoPP_FOUND)
    include_directories("${CRYPTOPP_INCLUDE_DIR}")

    message(STATUS "Crypto++ found!")
    message(STATUS "Crypto++ include: ${CRYPTOPP_INCLUDE_DIR}")
    message(STATUS "Crypto++ libs: ${CRYPTOPP_LIBRARY}")
endif()

if (O)
    if (O STREQUAL "0")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    elseif(O STREQUAL "1")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")
    elseif(O STREQUAL "2")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
    elseif(O STREQUAL "3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    endif()
endif()

set(CXX cxx_elements)
set(APP_CORE cxx_elements/core_application)
set(MOCKS_DIR cxx_tests/test_mocked_headres)

include_directories(${CXX}/common/Logger)
include_directories(${CXX}/common/Message)
include_directories(${CXX}/common/MessageGateway)
include_directories(${CXX}/common/Ping)
include_directories(${CXX}/common/JsonFileOperations)
include_directories(${CXX}/common/RunOptions)
include_directories(${APP_CORE}/MasterNode/Watchdog)
include_directories(${APP_CORE}/MasterNode/Master)
include_directories(${APP_CORE}/SlaveNode/Worker/WorkerBase)
include_directories(${APP_CORE}/SlaveNode/Worker/Decryptor)
include_directories(${APP_CORE}/SlaveNode/Slave)
include_directories(${CXX})

add_subdirectory(${CXX}/common)
add_subdirectory(${APP_CORE}/MasterNode)
add_subdirectory(${APP_CORE}/SlaveNode)

set(SOURCE_FILES ${APP_CORE}/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(
        ${PROJECT_NAME}
        ${MPI_LIBRARIES}
        ${Boost_LIBRARIES}
        ${CRYPTOPP_LIBRARY}
        RunOptionsLib
        MasterNodeObject
        SlaveNodeObject
        pthread)