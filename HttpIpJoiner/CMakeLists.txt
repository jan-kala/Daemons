cmake_minimum_required(VERSION 3.0.0)

project(HttpIpJoiner)

# Dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/CPM.cmake)

CPMAddPackage(
        NAME nlohmann_json
        GITHUB_REPOSITORY nlohmann/json
        VERSION 3.9.1)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Protobuf REQUIRED)
message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# =========================================

set(HttpIpJoiner_SRCS
        main.cpp
        MessageListeners/IFMonitorListener.cpp
        MessageListeners/IFMonitorListener.h
        MessageListeners/HttpDataReSenderListener.cpp
        MessageListeners/HttpDataReSenderListener.h
        ../Utils/LoggerCsv.cpp
        ../Utils/ProtobufReceiverBase.cpp
        Storage/Storage.h
        Storage/ActiveConnectionsPool.cpp
        Storage/ActiveConnectionsPool.h
        Storage/ServerEntry.cpp
        Storage/ServerEntry.h
        Storage/SocketEntry.cpp
        Storage/SocketEntry.h
        Dispatcher/Dispatcher.cpp
        Dispatcher/Dispatcher.h Storage/ListEntryBase.h)

# =========================================


add_executable(HttpIpJoiner ${HttpIpJoiner_SRCS})

target_link_libraries(HttpIpJoiner
    protobuf::libprotobuf
    -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
    libProtobufMessagesLibrary.a
)