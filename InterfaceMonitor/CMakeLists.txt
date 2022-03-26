# Minimum CMake required
cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake/)

# Project
project(InterfaceMonitor)

find_package(Protobuf REQUIRED)
find_package(PcapPlusPlus REQUIRED)

if(protobuf_VERBOSE)
    message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(InterfaceMonitor_SRCS main.cpp)
set(InterfaceMonitor_PROTOS ../HTTPDataReceiver/HTTPMessage.proto )

set(executable_name InterfaceMonitor)

add_executable(${executable_name} ${InterfaceMonitor_SRCS} ${InterfaceMonitor_PROTOS})
target_link_libraries(${executable_name}
        protobuf::libprotobuf
#        pcapplusplus::pcapplusplus
        )
# Generate protobuf messages
protobuf_generate(TARGET ${executable_name})