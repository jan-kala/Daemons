cmake_minimum_required(VERSION 3.0.0)

project(Joiner)

# Dependencies
include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/CPM.cmake)

## Downloading whole package takes too much space
#CPMAddPackage(
#        NAME nlohmann_json
#        GITHUB_REPOSITORY nlohmann/json
#        VERSION 3.9.1)

find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(Protobuf REQUIRED)
message(STATUS "Using Protocol Buffers ${protobuf_VERSION}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

# =========================================

set(Joiner_SRCS
        main.cpp
        MessageListeners/IFMonitorListener.cpp
        MessageListeners/IFMonitorListener.h
        MessageListeners/HttpDataReSenderListener.cpp
        MessageListeners/HttpDataReSenderListener.h
        ../Utils/LoggerCsv.cpp
        ../Utils/ProtobufReceiverBase.cpp
        ../Utils/Config.cpp
        Storage/Storage.h
        Storage/ConnectionPools/TlsConnectionPool.cpp
        Storage/ConnectionPools/TlsConnectionPool.h
        Storage/HistoryEntries/ServerEntry.cpp
        Storage/HistoryEntries/ServerEntry.h
        Storage/HistoryEntries/SocketEntry.cpp
        Storage/HistoryEntries/SocketEntry.h
        Dispatcher/Dispatcher.cpp
        Dispatcher/Dispatcher.h
        Storage/HistoryEntries/ListEntryBase.h
        Storage/ConnectionPools/HttpConnectionPool.cpp
        Storage/ConnectionPools/HttpConnectionPool.h
        Storage/Storage.cpp)

# =========================================

add_executable(Joiner ${Joiner_SRCS})

target_link_libraries(Joiner
    protobuf::libprotobuf
    nlohmann_json::nlohmann_json
    -L${CMAKE_CURRENT_SOURCE_DIR}/../ProtobufMessages/build
    libProtobufMessagesLibrary.a
)