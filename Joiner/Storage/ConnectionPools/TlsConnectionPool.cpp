//
// Created by Jan Kala on 21.04.2022.
//

#include "TlsConnectionPool.h"
#include "../Utils/LoggerCsv.h"
#include <numeric>

TlsConnectionPool::TlsConnectionPool(std::list<ServerEntry> *serverHistory, std::list<SocketEntry> *socketHistory,
                                     std::string archivePath)
    : serverHistory(serverHistory)
    , socketHistory(socketHistory)
    , archivePath(archivePath)
{}

TlsConnectionPool::ActionResult
TlsConnectionPool::add(annotator::IFMessage &msg) {
    auto serverKey = proto2serverKey(msg);
    auto serverName = msg.servername();
    auto socketKey = proto2socketKey(msg);

    // Create new SocketEntry and link it to SocketState
    auto newSocketEntry = SocketEntry::proto2socketEntry(msg);
    newSocketEntry.ts_start = msg.timestamp_packetcaptured();
    socketHistory->push_back(newSocketEntry);

    SocketState_t newSocketState;
    newSocketState.socketEntry = &(socketHistory->back());

    // Try to find pool entry
    auto pool_it =activeConnectionPool.find(serverKey);

    if (pool_it != activeConnectionPool.end()){

        // Add to the existing pool entry
        // Try to find server entry
        auto server_it = pool_it->second.find(serverName);

        if (server_it != pool_it->second.end()){
            // Server exists already
            // Check for existing socket
            auto socket_it = server_it->second.sockets.find(socketKey);

            if (socket_it == server_it->second.sockets.end()){
                server_it->second.serverEntry->sockets.push_back(&(socketHistory->back()));
                server_it->second.sockets.insert({socketKey, newSocketState});
                socketHistory->back().serverEntry = server_it->second.serverEntry;
            } else {
                // HTTP message using the same connection or
                // Duplicated client hello
                // so remove the last entry from history (we already have it there)
                socketHistory->pop_back();
            }

        } else {
            // Create ServerEntry, link with last socket from history and add to server history
            auto newServerEntry = ServerEntry::proto2serverEntry(msg);
            newServerEntry.sockets.push_back(&(socketHistory->back()));
            serverHistory->push_back(newServerEntry);
            socketHistory->back().serverEntry = &(serverHistory->back());

            ServerConnection_t newServerConnection;
            newServerConnection.serverEntry = &(serverHistory->back());
            newServerConnection.sockets.insert({socketKey, newSocketState});

            pool_it->second.insert({serverName, newServerConnection});
        }
    } else {
        auto newServerEntry = ServerEntry::proto2serverEntry(msg);
        newServerEntry.sockets.push_back(&(socketHistory->back()));
        serverHistory->push_back(newServerEntry);
        socketHistory->back().serverEntry = &(serverHistory->back());

        ServerConnection_t newServerConnection;
        newServerConnection.serverEntry = &(serverHistory->back());
        newServerConnection.sockets.insert({socketKey, newSocketState});

        ServerNames_t newServerNames;
        newServerNames.insert({serverName, newServerConnection});

        activeConnectionPool.insert({serverKey, newServerNames});
    }

    return ADDED;
}

TlsConnectionPool::ActionResult
TlsConnectionPool::addHttpRequestToServer(annotator::HttpMessage &msg){
    auto serverConnection = findServerConnection(msg);
    if (serverConnection){

        // Add requests to the archive
        serverConnection->serverEntry->requests.push_back(msg);
        auto archivedRequest = &serverConnection->serverEntry->requests.back();

        // Links request with the currently active connections
        for (auto socket: serverConnection->serverEntry->sockets){
            auto socketEntry = (SocketEntry *)socket;
            if (socketEntry->ts_end == 0) {
                // socket is active
                socketEntry->requests.push_back(archivedRequest);
            }
        }
        return PAIRED;
    }
    return NOP;
}

TlsConnectionPool::ActionResult
TlsConnectionPool::setFinOrRemove(annotator::IFMessage &msg) {
    ActionResult res = NOP;
    auto iters = findSocket(msg);
    auto pool_it = std::get<0>(iters);
    auto server_it = std::get<1>(iters);
    auto socket_it = std::get<2>(iters);

    // Set the correct flags to correct entry
    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end() &&
        socket_it != server_it->second.sockets.end()) {

        bool isMessageFromClient = proto2socketKey(msg) == socket_it->first;

        // Check the TCP flags
        if (msg.type() == annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN) {
            if (isMessageFromClient) {
                socket_it->second.tcp.FIN_client = true;
                res = TCP_FIN_CLIENT_SET;
            } else {
                socket_it->second.tcp.FIN_host = true;
                res = TCP_FIN_HOST_SET;
            }
        }

        // Check that modified connection isn't closed
        if (socket_it->second.tcp.FIN_client && socket_it->second.tcp.FIN_host) {
            // set the end of the socket connection
            socket_it->second.socketEntry->ts_end = msg.timestamp_packetcaptured();

            // remove closed connection from vector
            res = remove(pool_it, server_it, socket_it);
        }

    }
    return res;
}

TlsConnectionPool::ActionResult
TlsConnectionPool::remove(annotator::IFMessage &msg) {
    auto iters = findSocket(msg);
    auto pool_it = std::get<0>(iters);
    auto server_it = std::get<1>(iters);
    auto socket_it = std::get<2>(iters);

    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end() &&
        socket_it != server_it->second.sockets.end()) {

        // Set the end timestamp to the SocketEntry
        socket_it->second.socketEntry->ts_end = msg.timestamp_packetcaptured();
    }

    return remove(pool_it, server_it, socket_it);
}

TlsConnectionPool::ActionResult
TlsConnectionPool::remove(Pool_t ::iterator pool_it, ServerNames_t::iterator server_it, Sockets_t::iterator socket_it){
    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end() &&
        socket_it != server_it->second.sockets.end()) {

        // remove closed connection from pool
        server_it->second.sockets.erase(socket_it);

        if (server_it->second.sockets.empty()){
            // Archive server entry if it has some HTTP communication
            if (!server_it->second.serverEntry->requests.empty()) {
                server_it->second.serverEntry->print(archivePath.c_str());
            }

            // remove server entry
            pool_it->second.erase(server_it);
            if (pool_it->second.empty()){
                // remove pool entry
                activeConnectionPool.erase(pool_it);
            }
        }
        return REMOVED;
    }
    return NOP;
}

ServerKey_t
TlsConnectionPool::proto2serverKey(annotator::IFMessage &msg) {
    ServerKey_t connId;
    connId.ipVersion = msg.ipversion();
    connId.dstPort = msg.dstport();

    if (msg.ipversion() == 4){
        connId.dst.ipv4.s_addr = msg.dstv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(connId.dst.ipv6), msg.dstv6().data(), 16);
    }

    return connId;
}

ServerKey_t
TlsConnectionPool::proto2serverKey(annotator::HttpMessage &msg) {
    ServerKey_t connId;
    connId.ipVersion = msg.ipversion();
    connId.dstPort = msg.serverport();

    if (msg.ipversion() == 4){
        connId.dst.ipv4.s_addr = msg.serveripv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(connId.dst.ipv6), msg.serveripv6().data(), 16);
    }

    return connId;
}

ServerKey_t
TlsConnectionPool::proto2serverKeySwapped(annotator::IFMessage &msg) {
    ServerKey_t connId;
    connId.ipVersion = msg.ipversion();
    connId.dstPort = msg.srcport();

    if (msg.ipversion() == 4){
        connId.dst.ipv4.s_addr = msg.srcv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(connId.dst.ipv6), msg.srcv6().data(), 16);
    }

    return connId;
}

SocketKey_t
TlsConnectionPool::proto2socketKey(annotator::IFMessage &msg) {
    SocketKey_t connState;
    connState.ipVersion = msg.ipversion();
    connState.srcPort = msg.srcport();

    if (msg.ipversion() == 4){
        connState.src.ipv4.s_addr = msg.srcv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(connState.src.ipv6), msg.srcv6().data(), 16);
    }
    return connState;
}

SocketKey_t
TlsConnectionPool::proto2socketKeySwapped(annotator::IFMessage &msg) {
    SocketKey_t connState;
    connState.ipVersion = msg.ipversion();
    connState.srcPort = msg.dstport();

    if (msg.ipversion() == 4){
        connState.src.ipv4.s_addr = msg.dstv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(connState.src.ipv6), msg.dstv6().data(), 16);
    }
    return connState;
}

std::tuple<Pool_t::iterator,
        ServerNames_t::iterator,
        Sockets_t::iterator>
TlsConnectionPool::findSocket(ServerKey_t *connId, ServerNameKey_t *hostname, SocketKey_t *connState) {
    Pool_t::iterator pool_it;
    ServerNames_t::iterator server_it;
    Sockets_t ::iterator socket_it;

    if (connId) {

        pool_it = activeConnectionPool.find(*connId);
        if (pool_it != activeConnectionPool.end() && hostname) {

            server_it = pool_it->second.find(*hostname);
            if (server_it != pool_it->second.end() && connState) {

                socket_it = server_it->second.sockets.find(*connState);
            }
        }
    }
    return {pool_it, server_it, socket_it};
}

std::tuple<Pool_t::iterator,
        ServerNames_t::iterator,
        Sockets_t::iterator>
TlsConnectionPool::findSocket(annotator::IFMessage &msg){
    auto connId = proto2serverKey(msg);
    auto connState = proto2socketKey(msg);
    ServerNames_t ::iterator server_it;
    Sockets_t ::iterator socket_it;

    // Try to find the pool entry only
    auto iters = findSocket(&connId, nullptr, nullptr);
    auto pool_it = std::get<0>(iters);

    // If we haven't found anything, swap the IPs to try second direction
    if (pool_it == activeConnectionPool.end()){
        auto swappedConnId = proto2serverKeySwapped(msg);
        auto swappedConnState = proto2socketKeySwapped(msg);

        iters = findSocket(&swappedConnId, nullptr, nullptr);

        pool_it = std::get<0>(iters);
        connState = swappedConnState;
    }

    // Now we should have pointer to the correct pool entry
    if (pool_it != activeConnectionPool.end()) {

        // Loop through all the sockets since we don't know the hostname
        for (server_it = pool_it->second.begin(); server_it != pool_it->second.end(); server_it++) {
            socket_it = server_it->second.sockets.find(connState);
            if (socket_it != server_it->second.sockets.end()) {
                break;
            }
        }
    }

    return {pool_it, server_it, socket_it};
}

ServerConnection_t *
TlsConnectionPool::findServerConnection(annotator::HttpMessage &msg){
    auto connId = proto2serverKey(msg);
    auto hostname = msg.hostname();
    auto iters = findSocket(&connId, &hostname, nullptr);
    auto pool_it = std::get<0>(iters);
    auto server_it = std::get<1>(iters);

    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end()){
        return &(server_it->second);
    }
    return nullptr;

}

void
TlsConnectionPool::printPool() {

    std::cout << PRINT_OFFSET << "-- Connections pool: " <<std::endl;
    for (auto poolItem: activeConnectionPool){
        char ipv4[INET_ADDRSTRLEN];
        char ipv6[INET6_ADDRSTRLEN];
        std::string ip;

        if (poolItem.first.ipVersion == 4){
            inet_ntop(AF_INET, &(poolItem.first.dst.ipv4.s_addr), ipv4, INET_ADDRSTRLEN);
            ip = ipv4;

        } else if (poolItem.first.ipVersion == 6){
            inet_ntop(AF_INET6, &(poolItem.first.dst.ipv6), ipv6, INET6_ADDRSTRLEN);
            ip = ipv6;
        }
        std::cout << PRINT_OFFSET << ip << ":" << poolItem.first.dstPort << std::endl;

        for (const auto& serverItem: poolItem.second){
            std::cout<< PRINT_OFFSET << "  | " << serverItem.first << std::endl;
            for (auto socketItem: serverItem.second.sockets){
                if (socketItem.first.ipVersion == 4){
                    inet_ntop(AF_INET, &(socketItem.first.src.ipv4.s_addr), ipv4, INET_ADDRSTRLEN);
                    ip = ipv4;
                } else if (socketItem.first.ipVersion == 6){
                    inet_ntop(AF_INET6, &(socketItem.first.src.ipv6), ipv6, INET6_ADDRSTRLEN);
                    ip = ipv6;
                }
                std::cout << PRINT_OFFSET << "    | "
                          << socketItem.second.tcp.FIN_host << socketItem.second.tcp.FIN_client
                          << " " << ip << " : " << socketItem.first.srcPort << std::endl;
            }
        }
    }
}


