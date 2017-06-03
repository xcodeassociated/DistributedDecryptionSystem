cmake_minimum_required(VERSION 3.5.1)
project(SimpleCrypt)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/cryptopp-cmake")

find_package(Boost COMPONENTS program_options chrono thread system REQUIRED) ## BOOST version can be specified as 2nd argument
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(SOURCE_FILES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${CRYPTOPP_LIBRARY} ${Boost_LIBRARIES})