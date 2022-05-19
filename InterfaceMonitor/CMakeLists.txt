# Minimum CMakeModules required
cmake_minimum_required(VERSION 3.0.0)

# Project name
project(InterfaceMonitor)

find_package(Protobuf REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

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
        ../Utils/ProtobufSenderBase.cpp
        ../Utils/Config.cpp)

# =========================================

set(executable_name InterfaceMonitor)

add_executable(${executable_name} ${InterfaceMonitor_SRCS})
target_link_libraries(${executable_name}
        protobuf::libprotobuf
        nlohmann_json::nlohmann_json
        -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
        libProtobufMessagesLibrary.a
        ${PCAP_LIBRARY}
)