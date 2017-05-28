########################################################################################################################
## DDS Project CMakeLists.txt - Main app config file
##
## TODO: Clean CMakeLists.txt to make it more generic for use without cmake hardcoded env variables
##
########################################################################################################################

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

message (STATUS "Build DIR: ${CMAKE_CURRENT_BINARY_DIR}")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fext-numeric-literals")

    message(STATUS "Linux")

    if (ARM)
        ## Linux amdhf only config

        ## doesn't work since Clion 2017.1 - Need to manually pass the C and CXX compiler as cmake argument:
        #  -DCMAKE_C_COMPILER=<C_COMPILER> -DCMAKE_CXX_COMPILER=<CXX_COMPILER>
        #
        #        set(CROSS_COMPILER_PATH /usr/bin/)
        #        set(TOOLCHAIN_PREFIX ${CROSS_COMPILER_PATH}arm-linux-gnueabihf)
        #        set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}-gcc)
        #        set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}-g++)

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
        find_package(Boost COMPONENTS mpi serialization chrono thread system REQUIRED) ## BOOST version can be specified as 2nd argument
        if(Boost_FOUND)
            message(STATUS "ARM_BOOST Found!")
            message(STATUS "ARM_Boost version: ${Boost_VERSION}")
            message(STATUS "ARM_Boost include: ${Boost_INCLUDE_DIRS}")
        endif()

        set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/include/openmpi/")
        set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/openmpi/include/")
        set (CMAKE_PREFIX_PATH "/home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/openmpi/lib/")

        find_package(MPI REQUIRED)
        if(MPI_FOUND)
            message(STATUS "ARM_MPI Found!")
        endif()

        include_directories("/home/jm/arm-linux-gnueabihf-pi/usr/include/")
        include_directories(${MPI_INCLUDE_PATH})
        include_directories(${Boost_INCLUDE_DIRS})

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread ")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/jm/arm-linux-gnueabihf-pi/lib/")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ldl -fPIC -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf/openmpi/lib -Wl,--enable-new-dtags -L/usr/lib/arm-linux-gnueabihf/openmpi/lib")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath -Wl,/usr/lib/arm-linux-gnueabihf/")

        # to link shared library when running on cluster
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,-rpath,/mpi/DDS")

    else()
        ## Linux amd64 only configuration

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

        find_package(Boost COMPONENTS mpi serialization program_options chrono thread system REQUIRED) ## BOOST version can be specified as 2nd argument
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

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")


    endif()

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    ## macOS x64 only config
    message(STATUS "macOS")

    set (CMAKE_PREFIX_PATH "/usr/include")
    set (CMAKE_PREFIX_PATH "/usr/lib/")
    set (CMAKE_PREFIX_PATH "/usr/local/opt/")

    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/cryptopp/5.6.5/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/cryptopp/5.6.5/include/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/cryptopp/5.6.5/lib/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/open-mpi/1.10.2_1/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/open-mpi/1.10.2_1/include/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/open-mpi/1.10.2_1/lib/")

    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/boost/1.60.0_2/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/boost/1.60.0_2/include/")
    set (CMAKE_PREFIX_PATH "/usr/local/Cellar/boost/1.60.0_2/lib/")

    set (BOOST_ROOT "/usr/local/Cellar/boost/1.60.0_2/")

    include_directories(${MPI_INCLUDE_PATH})
    include_directories(${Boost_INCLUDE_DIRS})
    include_directories("/usr/local/Cellar/cryptopp/5.6.5/include/")

    find_package(MPI REQUIRED)
    if(MPI_FOUND)
        message(STATUS "MPI Found!")
        message(STATUS "MPI include: ${MPI_INCLUDE_PATH}")
    endif()

    find_package(Boost COMPONENTS mpi serialization program_options chrono thread system REQUIRED) ## BOOST version can be specified as 2nd argument
    if(Boost_FOUND)
        message(STATUS "Boost Found!")
        message(STATUS "Boost version: ${Boost_VERSION}")
        message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")
        message(STATUS "Boost libs: ${Boost_LIBRARY_DIRS}")
    endif()

    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/cryptopp-cmake")
    find_package(CryptoPP REQUIRED)

    if (CryptoPP_FOUND)
        message(STATUS "Crypto++ found!")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

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
        ## by default turn off optimization - for debugging
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
    endif()
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
    ## TODO: Make ARM cross-compiler able to use Boost cmake module to link BOOST libs
    target_link_libraries(${PROJECT_NAME}
            ${MPI_LIBRARIES}
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_mpi.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_serialization.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_program_options.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_thread.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_chrono.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libboost_system.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libcrypto++.so.6
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libcryptopp.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libhwloc.so
            /home/jm/arm-linux-gnueabihf-pi/usr/lib/arm-linux-gnueabihf/libhwloc.so.5
            /home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/libudev.so.1
            /home/jm/arm-linux-gnueabihf-pi/lib/arm-linux-gnueabihf/libpthread.so.0
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
                    copy ${CMAKE_CURRENT_BINARY_DIR}/cxx_elements/core_application/SlaveNode/libSlaveNodeObject.so ${CP})
    endif()
endif()