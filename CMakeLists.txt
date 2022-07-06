cmake_minimum_required(VERSION 3.17)
project(zmqbroker)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

find_library(libzmq
        NAMES zmq
        HINTS /usr/local/lib)
find_package(Boost COMPONENTS json REQUIRED)

add_executable(zmqbroker main.cpp)
target_link_libraries(zmqbroker PUBLIC ${libzmq} ${Boost_JSON_LIBRARY})
