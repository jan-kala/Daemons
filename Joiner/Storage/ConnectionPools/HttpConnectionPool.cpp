//
// Created by Jan Kala on 15.05.2022.
//

#include "HttpConnectionPool.h"

HttpConnectionPool::HttpConnectionPool(std::list<ServerEntry> *serverHistory, std::list<SocketEntry> *socketHistory)
        : serverHistory(serverHistory)
        , socketHistory(socketHistory)
{}

void HttpConnectionPool::add(annotator::IFMessage &msg) {
    // Create and Insert socket entry into history
    auto socketEntry = SocketEntry::proto2socketEntry(msg);
    socketHistory->push_back(socketEntry);

    // Create Server entry, link with socket and insert into history
    auto serverEntry = ServerEntry::proto2serverEntry(msg);
    serverEntry.sockets.push_back(&(socketHistory->back()));
    serverHistory->push_back(serverEntry);

    // Link socket entry with server entry
    socketHistory->back().serverEntry = &(serverHistory->back());
}

