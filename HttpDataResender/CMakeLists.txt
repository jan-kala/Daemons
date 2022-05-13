# Minimum CMake required
cmake_minimum_required(VERSION 3.0.0)

# Project
project(HttpDataReSender)

# Dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/CPM.cmake)

CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.9.1)

find_package(Protobuf REQUIRED)
message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")

find_package(nlohmann_json 3.2.0 REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_CXX_STANDARD 17)

# =========================================

set(HttpDataReSender_SRCS
        main.cpp
        HttpDataReSender.cpp HttpDataReSender.h
        ../Utils/ProtobufSenderBase.cpp
        ../Utils/LoggerCsv.cpp)

# =========================================

set(executable_name HttpDataReSender)

add_executable(${executable_name} ${HttpDataReSender_SRCS})
target_link_libraries(${executable_name}
        protobuf::libprotobuf
        nlohmann_json::nlohmann_json
        -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
        libProtobufMessagesLibrary.a
)