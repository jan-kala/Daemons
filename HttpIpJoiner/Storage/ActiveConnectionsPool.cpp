//
// Created by Jan Kala on 21.04.2022.
//

#include "ActiveConnectionsPool.h"
#include "../Utils/LoggerCsv.h"

void
ActiveConnectionsPool::processMessage(annotator::IFMessage &message) {
    ActionResult res;
    switch (message.type()) {
        case annotator::IFMessage_MessageType_TLS_NEW_CONNECTION:
            res = add(message);
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN:
            res = setFinOrRemove(message);
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST:
            res = remove(message);
            break;
        default:
            break;
    }
    return;

    switch (res) {
        case ADDED:
            std::cout<<PRINT_OFFSET<<"-Added:" << std::endl;
            break;
        case REMOVED:
            std::cout<<PRINT_OFFSET<<"-Removed:" <<std::endl;
            break;
        case NOP:
            std::cout<<PRINT_OFFSET<<"-Nop" <<std::endl;
            break;
        case TCP_FIN_HOST_SET:
            std::cout<<PRINT_OFFSET<<"-TCP client set" <<std::endl;
            break;
        case TCP_FIN_CLIENT_SET:
            std::cout<<PRINT_OFFSET<<"-TCP host set" <<std::endl;
            break;
    }

}

void ActiveConnectionsPool::processMessage(annotator::HttpMessage &msg) {
    ActionResult res;
    switch (msg.type()){
        case annotator::HttpMessage_MessageType_NEW_REQUEST:
            break;

        case annotator::HttpMessage_MessageType_PAIRING:
            res = addHttpRequestToServer(msg);
            break;

        default:
            break;
    }

    switch (res) {
        case PAIRED:
//            std::cout << "PAIRED! " << msg.hostname() << std::endl;
            succ++;
            break;
        default:
//            std::cout << "Failed to pair: " << (msg.protocol() == annotator::HttpMessage_RequestProtocol_HTTP ? "http:" : "https:")  << msg.hostname() << std::endl;
//            LoggerCsv::log(msg);
            failed++;
            break;
    }

//    auto rate = (float(succ) / (float(succ) + float(failed))) * 100;
//    std::cout << "rate: " << rate << "%" << std::endl;

}

ActiveConnectionsPool::ActionResult
ActiveConnectionsPool::add(annotator::IFMessage &msg) {
    auto serverKey = proto2serverKey(msg);
    auto serverName = msg.servername();
    auto socketKey = proto2socketKey(msg);

    // Create new SocketEntry and link it to SocketState
    auto newSocketEntry = proto2socketEntry(msg);
    newSocketEntry.ts_start = msg.timestamp_s();
    socketHistory.connections.push_back(newSocketEntry);

    SocketState_t newSocketState;
    newSocketState.socketEntry = &(socketHistory.connections.back());

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
                server_it->second.serverEntry->sockets.push_back(&(socketHistory.connections.back()));
                server_it->second.sockets.insert({socketKey, newSocketState});
            } else {
                // Duplicated client hello, remove the last entry from history (we already have it there)
                socketHistory.connections.pop_back();
            }

        } else {
            // Create ServerEntry, link with last socket from history and add to server history
            auto newServerEntry = proto2serverEntry(msg);
            newServerEntry.sockets.push_back(&(socketHistory.connections.back()));
            serverHistory.connections.push_back(newServerEntry);

            ServerConnection_t newServerConnection;
            newServerConnection.serverEntry = &(serverHistory.connections.back());
            newServerConnection.sockets.insert({socketKey, newSocketState});

            pool_it->second.insert({serverName, newServerConnection});
        }
    } else {
        auto newServerEntry = proto2serverEntry(msg);
        newServerEntry.sockets.push_back(&(socketHistory.connections.back()));
        serverHistory.connections.push_back(newServerEntry);

        ServerConnection_t newServerConnection;
        newServerConnection.serverEntry = &(serverHistory.connections.back());
        newServerConnection.sockets.insert({socketKey, newSocketState});

        ServerNames_t newServerNames;
        newServerNames.insert({serverName, newServerConnection});

        activeConnectionPool.insert({serverKey, newServerNames});
    }

    return ADDED;
}

ActiveConnectionsPool::ActionResult
ActiveConnectionsPool::setFinOrRemove(annotator::IFMessage &msg) {
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
            socket_it->second.socketEntry->ts_end = msg.timestamp_s();

            // remove closed connection from vector
            res = remove(pool_it, server_it, socket_it);
        }

    }
    return res;
}

ActiveConnectionsPool::ActionResult
ActiveConnectionsPool::remove(annotator::IFMessage &msg) {
    auto iters = findSocket(msg);
    auto pool_it = std::get<0>(iters);
    auto server_it = std::get<1>(iters);
    auto socket_it = std::get<2>(iters);

    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end() &&
        socket_it != server_it->second.sockets.end()) {

        // Set the end timestamp to the SocketEntry
        socket_it->second.socketEntry->ts_end = msg.timestamp_s();
    }

    return remove(pool_it, server_it, socket_it);
}

ActiveConnectionsPool::ActionResult
ActiveConnectionsPool::remove(Pool_t ::iterator pool_it, ServerNames_t::iterator server_it, Sockets_t::iterator socket_it){
    if (pool_it != activeConnectionPool.end() &&
        server_it != pool_it->second.end() &&
        socket_it != server_it->second.sockets.end()) {

        // remove closed connection from vector
        server_it->second.sockets.erase(socket_it);

        if (server_it->second.sockets.empty()){
            // DEBUG PRINT
            auto entry = server_it->second.serverEntry;
            ServerConnectionList::print(entry);

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
ActiveConnectionsPool::proto2serverKey(annotator::IFMessage &msg) {
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
ActiveConnectionsPool::proto2serverKey(annotator::HttpMessage &msg) {
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
ActiveConnectionsPool::proto2serverKeySwapped(annotator::IFMessage &msg) {
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
ActiveConnectionsPool::proto2socketKey(annotator::IFMessage &msg) {
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
ActiveConnectionsPool::proto2socketKeySwapped(annotator::IFMessage &msg) {
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

SocketConnectionList::SocketEntry
ActiveConnectionsPool::proto2socketEntry(annotator::IFMessage &msg){
    SocketConnectionList::SocketEntry newEntry;
    newEntry.ipVersion = msg.ipversion();
    if (msg.ipversion() == 4){
        newEntry.src.ipv4.s_addr = msg.srcv4();
        newEntry.dst.ipv4.s_addr = msg.dstv4();
    } else if (msg.ipversion() == 6){
        memcpy(&(newEntry.src.ipv6) ,msg.srcv6().data(), 16);
        memcpy(&(newEntry.dst.ipv6) ,msg.dstv6().data(), 16);
    }
    newEntry.srcPort = msg.srcport();
    newEntry.dstPort = msg.dstport();

    return newEntry;
}

ServerConnectionList::ServerEntry
ActiveConnectionsPool::proto2serverEntry(annotator::IFMessage &msg){
    ServerConnectionList::ServerEntry newEntry;
    newEntry.serverNameIndicator = msg.servername();
    return newEntry;
}

std::tuple<Pool_t::iterator,
        ServerNames_t::iterator,
        Sockets_t::iterator>
ActiveConnectionsPool::findSocket(ServerKey_t *connId, ServerNameKey_t *hostname, SocketKey_t *connState) {
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
ActiveConnectionsPool::findSocket(annotator::IFMessage &msg){
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
ActiveConnectionsPool::findServerConnection(annotator::HttpMessage &msg){
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

ActiveConnectionsPool::ActionResult
ActiveConnectionsPool::addHttpRequestToServer(annotator::HttpMessage &msg){
    auto serverConnection = findServerConnection(msg);
    if (serverConnection){
        serverConnection->serverEntry->requests.push_back(msg);
        return PAIRED;
    }
    return NOP;
}

void
ActiveConnectionsPool::printPool() {

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
