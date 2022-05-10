cmake_minimum_required(VERSION 3.21)

project(HttpIpJoiner)

find_package(Protobuf REQUIRED)
message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# =========================================

set(HttpIpJoiner_SRCS
        main.cpp
        IFMonitorListener.cpp
        HttpDataReSenderListener.cpp
        ../Utils/LoggerCsv.cpp
        ../Utils/ProtobufReceiverBase.cpp
        Storage/Storage.h
        Storage/ActiveConnectionsPool.cpp
        Storage/ActiveConnectionsPool.h
        Storage/ServerConnectionList.cpp
        Storage/ServerConnectionList.h
        Storage/SocketConnectionList.cpp
        Storage/SocketConnectionList.h)

# =========================================


add_executable(HttpIpJoiner ${HttpIpJoiner_SRCS})

target_link_libraries(HttpIpJoiner
    protobuf::libprotobuf
    -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
    libProtobufMessagesLibrary.a
)