cmake_minimum_required(VERSION 3.5.1)
project(DDS)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Source DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

## cmake options
option(ARM "Build for armhf" OFF)

option(O "Optimalize" 0)

option(CP "copy exec to given location" "")

set(CMAKE_VERBOSE_MAKEFILE on)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals")

message (STATUS "Build DIR: ${CMAKE_CURRENT_BINARY_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    message(STATUS "Linux")

    if (ARM)
        ## set optimization for ARM - to run faster on my own 'develop' cluster.
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

        ## armhf only configuration
        #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

        set(CROSS_COMPILER_PATH /usr/bin/)
        set(TOOLCHAIN_PREFIX ${CROSS_COMPILER_PATH}arm-linux-gnueabihf)
        set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
        set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

        set (CMAKE_PREFIX_PATH "/usr/arm-boost/")
        set (CMAKE_PREFIX_PATH "/usr/arm-boost/include/")
        set (CMAKE_PREFIX_PATH "/usr/arm-boost/lib")

        set (CMAKE_PREFIX_PATH "/usr/arm-cryptopp/include")
        set (CMAKE_PREFIX_PATH "/usr/arm-cryptopp/lib")


        set (BOOST_ROOT "/usr/arm-boost/")
        find_package(Boost 1.6.0 COMPONENTS mpi serialization REQUIRED)
        if(Boost_FOUND)
            message(STATUS "ARM_BOOST Found!")
            message(STATUS "ARM_Boost version: ${Boost_VERSION}")
            message(STATUS "ARM_Boost include: ${Boost_INCLUDE_DIRS}")
        endif()

        set (CMAKE_PREFIX_PATH "/usr/arm-openmpi/")
        set (CMAKE_PREFIX_PATH "/usr/arm-openmpi/include")
        set (CMAKE_PREFIX_PATH "/usr/arm-openmpi/lib")

        find_package(MPI REQUIRED)
        if(MPI_FOUND)
            message(STATUS "ARM_MPI Found!")
        endif()

        include_directories("/usr/arm-openmpi/include")
        include_directories(${MPI_INCLUDE_PATH})
        include_directories(${Boost_INCLUDE_DIRS})
        include_directories("/usr/arm-cryptopp/include")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -fPIC -Wl,-rpath -Wl,/home/pi/.linuxbrew/opt/libevent/lib -Wl,-rpath -Wl,/home/pi/.linuxbrew/Cellar/open-mpi/1.10.2_1/lib -Wl,--enable-new-dtags -L/home/pi/.linuxbrew/Cellar/open-mpi/1.10.2_1/lib")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/arm-cryptopp/lib -lcryptopp")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf/")

        ## to link shared library when running on cluster
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/mpi/zzz")


    else()
        ## amd64 only configuration

        set (CMAKE_PREFIX_PATH "/usr/include")
        set (CMAKE_PREFIX_PATH "/usr/include/cryptopp/")
        set (CMAKE_PREFIX_PATH "/usr/include/mpi")
        set (CMAKE_PREFIX_PATH "/usr/include/openmpi")
        set (CMAKE_PREFIX_PATH "/usr/include/boost")

        set (CMAKE_PREFIX_PATH "/usr/lib/")
        set (CMAKE_PREFIX_PATH "/usr/lib/x86_64-linux-gnu/")

        include_directories("/usr/include/mpi")
        include_directories(${MPI_INCLUDE_PATH})
        include_directories(${Boost_INCLUDE_DIRS})
        include_directories("/usr/include/cryptopp/")

        find_package(MPI REQUIRED)
        if(MPI_FOUND)
            message(STATUS "MPI Found!")
            message(STATUS "MPI include: ${MPI_INCLUDE_PATH}")
        endif()

        find_package(Boost 1.5.8 COMPONENTS mpi serialization program_options REQUIRED)
        if(Boost_FOUND)
            message(STATUS "Boost Found!")
            message(STATUS "Boost version: ${Boost_VERSION}")
            message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
            message(STATUS "Boost libs: ${Boost_LIBRARY_DIRS}")
        endif()

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath -Wl,/usr/lib/openmpi/lib -Wl,--enable-new-dtags")

        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/cryptopp-cmake")
        find_package(CryptoPP REQUIRED)

        if (CryptoPP_FOUND)
            message(STATUS "Crypto++ found!")
        endif()

    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

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
include_directories(${APP_CORE}/MasterNode/MainClass)
include_directories(${APP_CORE}/SlaveNode/MainClass)
include_directories(${CXX})

set(MOCKS_DIR cxx_tests/test_source/mocks_headres)

add_subdirectory(${CXX})

add_subdirectory(cxx_tests)

set(SOURCE_FILES ${CXX}/main.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if (ARM)
    target_link_libraries(${PROJECT_NAME}
            ${MPI_LIBRARIES}
            /usr/arm-boost/lib/libboost_mpi.so
            /usr/arm-boost/lib/libboost_serialization.so
            /usr/arm-boost/lib/libboost_program_options.so
            /usr/arm-cryptopp/lib/libcrypto++.so.6
            /usr/arm-cryptopp/lib/libcryptopp.so
            pthread
            MasterNodeObject
            SlaveNodeObject)
else()
    target_link_libraries(${PROJECT_NAME}
            ${MPI_LIBRARIES}
            ${Boost_LIBRARIES}
            ${CRYPTOPP_LIBRARY}
            pthread
            MasterNodeObject
            SlaveNodeObject)
endif()

## options checks
if (NOT CP STREQUAL "")
    add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CP}

            ## Copy also SHARED LIBs
            COMMAND ${CMAKE_COMMAND} -E
                copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/MasterNode/libMasterNodeObject.so ${CP}
            COMMAND ${CMAKE_COMMAND} -E
                copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/SlaveNode/libSlaveNodeObject.so ${CP})

endif()
