# Minimum CMake required
cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Project
project(HTTPDataResender)

find_package(Protobuf REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

if(protobuf_VERBOSE)
    message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(Resender_SRCS main.cpp)
set(Resender_PROTOS HTTPMessage.proto)

set(executable_name HTTPDataResender)

add_executable(${executable_name} ${Resender_SRCS} ${Resender_PROTOS})
target_link_libraries(${executable_name}
        protobuf::libprotobuf
        nlohmann_json::nlohmann_json
        )
# Generate protobuf messages
protobuf_generate(TARGET HTTPDataResender)