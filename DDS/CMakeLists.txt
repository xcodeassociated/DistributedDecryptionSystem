########################################################################################################################
## DDS Project CMakeLists.txt - Main app config file
##
## TODO: Clean CMakeLists.txt to make it more generic for use without cmake hardcoded env variables
## TODO: Remove my local ARM CMAKE setup - this is for testing only
########################################################################################################################

cmake_minimum_required(VERSION 3.5.1)
project(DDS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Source DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

option(ARM "Build for armhf" OFF)
option(O "Optimalize" 0)
option(CP "copy exec to given location" "")
option(GTEST_DIR "GoogleTest lib directory")

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

message (STATUS "Build DIR: ${CMAKE_CURRENT_BINARY_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals")
endif()

if (ARM)
    message(STATUS "ARCH: arm")

    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/lib/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/")

    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/include/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/lib/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/arm-linux-gnueabihf/")

    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/include/cryptopp/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/include/crypto++/")

    set (BOOST_ROOT "/home/jm/arm-linux-gnueabihf-pi/usr/")
    set (BOOST_LIBRARYDIR "/home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf")

    include_directories("/home/jm/arm-linux-gnueabihf-pi/usr/include/")

    find_package(Boost COMPONENTS mpi serialization chrono thread system REQUIRED)
    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})

        message(STATUS "ARM_BOOST Found!")
        message(STATUS "ARM_Boost version: ${Boost_VERSION}")
        message(STATUS "ARM_Boost include: ${Boost_INCLUDE_DIRS}")
        message(STATUS "ARM_Boost libs: ${Boost_INCLUDE_DIRS}")
    endif()

    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/include/openmpi/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/openmpi/include/")
    set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/openmpi/lib/")

    find_package(MPI REQUIRED)
    if(MPI_FOUND)
        include_directories(${MPI_INCLUDE_PATH})

        message(STATUS "ARM_MPI Found!")
        message(STATUS "ARM_MPI include: ${MPI_INCLUDE_PATH}")
        message(STATUS "ARM_MPI libs: ${MPI_LIBRARIES}")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/jm/arm-linux-gnueabihf-pi/lib/")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -fPIC -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf/openmpi/lib -Wl,--enable-new-dtags -L/usr/lib/arm-linux-gnueabihf/openmpi/lib")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf/")

    # to link shared library when running on cluster
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/mpi/DDS")

else()
    message(STATUS "ARCH: amd64")

    find_package(MPI REQUIRED)
    if(MPI_FOUND)
        include_directories(${MPI_INCLUDE_PATH})

        message(STATUS "MPI Found!")
        message(STATUS "MPI include: ${MPI_INCLUDE_PATH}")
        message(STATUS "MPI libs: ${MPI_LIBRARIES}")
    endif()

    find_package(Boost COMPONENTS mpi serialization program_options chrono thread system filesystem REQUIRED)
    if(Boost_FOUND)
        include_directories(${Boost_INCLUDE_DIRS})

        message(STATUS "Boost Found!")
        message(STATUS "Boost version: ${Boost_VERSION}")
        message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
        message(STATUS "Boost libs: ${Boost_LIBRARY_DIRS}")
    endif()

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/cryptopp-cmake")
    find_package(CryptoPP REQUIRED)

    if (CryptoPP_FOUND)
        include_directories("${CRYPTOPP_INCLUDE_DIR}")

        message(STATUS "Crypto++ found!")
        message(STATUS "Crypto++ include: ${CRYPTOPP_INCLUDE_DIR}")
        message(STATUS "Crypto++ libs: ${CRYPTOPP_LIBRARY}")
    endif()
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

set(APP_CORE cxx_elements/core_application)
set(CXX cxx_elements)

include_directories(${CXX}/common/Logger)
include_directories(${CXX}/common/Message)
include_directories(${CXX}/common/MessageGateway)
include_directories(${CXX}/common/Ping)
include_directories(${APP_CORE}/MasterNode/Watchdog)
include_directories(${APP_CORE}/MasterNode/Master)
include_directories(${APP_CORE}/SlaveNode/Worker/WorkerBase)
include_directories(${APP_CORE}/SlaveNode/Worker/Decryptor)
include_directories(${APP_CORE}/SlaveNode/Slave)
include_directories(${CXX})

set(MOCKS_DIR cxx_tests/test_mocked_headres)
add_subdirectory(${CXX}/common)
add_subdirectory(${APP_CORE}/MasterNode)
add_subdirectory(${APP_CORE}/SlaveNode)
add_subdirectory(cxx_tests)

set(SOURCE_FILES ${APP_CORE}/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (ARM)
    target_link_libraries(
            ${PROJECT_NAME}
            ${MPI_LIBRARIES}
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_mpi.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_serialization.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_program_options.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_thread.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_chrono.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_system.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_mpi.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_filesystem.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libcrypto++.so.6
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libcryptopp.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libhwloc.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libhwloc.so.5
            /home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/libudev.so.1
            /home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/libpthread.so.0
            LoggerLib
            MasterNodeObject
            SlaveNodeObject)
else()
    target_link_libraries(
            ${PROJECT_NAME}
            ${MPI_LIBRARIES}
            ${Boost_LIBRARIES}
            ${CRYPTOPP_LIBRARY}
            pthread
            LoggerLib
            MasterNodeObject
            SlaveNodeObject)
endif()

## post build steps
if (CP)
    if (NOT CP STREQUAL "")
        add_custom_command(TARGET ${PROJECT_NAME}
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CP}

                ## Copy also SHARED LIBs
                COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/MasterNode/libMasterNodeObject.so ${CP}
                COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/SlaveNode/libSlaveNodeObject.so ${CP}
                COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/common/libLoggerLib.so ${CP}
                COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/MasterNode/libWatchdogLib.so ${CP}
                COMMAND ${CMAKE_COMMAND} -E
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/SlaveNode/Worker/libDecryptorLib.so ${CP}
                )

    endif()
endif()