# Minimum CMake required
cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Project
project(HTTPDataResender)

find_package(Protobuf REQUIRED)

if(protobuf_VERBOSE)
    message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

add_library(proto)

protobuf_generate(
        LANGUAGE cpp
        TARGET proto
        PROTOS HTTPMessage.proto)