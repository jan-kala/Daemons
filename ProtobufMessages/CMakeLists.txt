# Minimum CMakeModules required
cmake_minimum_required(VERSION 3.0.0)

# Add CMakeModules modules for finding packages, etc.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules/)

project(ProtobufMessagesLibrary)

find_package(Protobuf REQUIRED)

message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")


set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
#===========================
set(ProtobufMessages_Sources
        IFMonitorMessage.proto
        HTTPMessage.proto)

#===========================

add_library(ProtobufMessagesLibrary)
target_link_libraries(ProtobufMessagesLibrary
        protobuf::libprotobuf)

protobuf_generate(
        TARGET ProtobufMessagesLibrary
        PROTOS ${ProtobufMessages_Sources})