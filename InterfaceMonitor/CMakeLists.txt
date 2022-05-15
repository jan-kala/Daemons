# Minimum CMakeModules required
cmake_minimum_required(VERSION 3.0.0)

# Project name
project(InterfaceMonitor)

find_package(Protobuf REQUIRED)
message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")

include(CMakeModules/FindPCAP.cmake)

set(CMAKE_CXX_FLAGS "-lpcap")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# =========================================

set(InterfaceMonitor_SRCS
        main.cpp
        InterfaceMonitor.h
        InterfaceMonitor.cpp
        ../Utils/LoggerCsv.cpp
        ../Utils/ProtobufSenderBase.cpp)

# =========================================

set(executable_name InterfaceMonitor)

add_executable(${executable_name} ${InterfaceMonitor_SRCS})
target_link_libraries(${executable_name}
        protobuf::libprotobuf
        -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
        libProtobufMessagesLibrary.a
        ${PCAP_LIBRARY}
)