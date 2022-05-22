//
// Created by Jan Kala on 13.05.2022.
//

#include "Dispatcher.h"
#include <unistd.h>

Dispatcher::Dispatcher(Config& config, Storage *storage)
    : storage(storage)
    , config(config)
{}

Dispatcher::~Dispatcher() {
    worker_cv.notify_all();
    if (worker_thread.joinable()){
        worker_thread.join();
    }
}

void Dispatcher::run() {
    worker_thread = std::thread(&Dispatcher::worker, this);
}

void Dispatcher::worker() {
    std::unique_lock<std::mutex> lock(worker_mutex);
    std::chrono::microseconds us(10);

    createSocket();

    while (worker_cv.wait_for(lock, us) == std::cv_status::timeout){
        dispatchRequest();
    }

}

void Dispatcher::createSocket() {
    int newSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (newSockFd == 0) {
        throw std::runtime_error("Dispatcher: Failed to create dispatcher socket!");
    }

    int opt = 1;
    if (setsockopt(newSockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        throw std::runtime_error("Dispatcher: Failed to set socket options!");
    }

    auto port = config[CONFIG_JOINER_DISPATCHER_PORT].get<uint32_t>();
    auto ip = config[CONFIG_JOINER_DISPATCHER_IP].get<std::string>();

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip.c_str(), &(addr.sin_addr.s_addr));
    addr.sin_port = htons(port);

    if (bind(newSockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        throw std::runtime_error("Dispatcher: Failed to bind to the socket!");
    }

    sockFd = newSockFd;
}

void Dispatcher::dispatchRequest() {

    if (listen(sockFd, 100) < 0) {
        throw std::runtime_error("Dispatcher: Failed to listen for requests!");
    }

    struct sockaddr_in addr = {};
    int addrLen = sizeof(addr);
    auto acceptSocket = accept(sockFd, (struct sockaddr*)&addr, (socklen_t*)&addrLen);
    if (acceptSocket < 0){
        throw std::runtime_error("Dispatcher: Failed to create communication socket!");
    }

    std::string strMessage;
    try {
        strMessage = readRequest(acceptSocket);
    } catch (SocketDisconnected& e){
        std::cerr<<"Dispatcher: Request hasn't been dispatched, client disconnected." << std::endl;
        return;
    }
    json jsonMessage = json::parse(strMessage);
    auto response = processRequest(jsonMessage);
    sendResponse(acceptSocket, response);

    close(acceptSocket);

}

std::string Dispatcher::readRequest(int acceptSocket) {
    size_t dataLen = 0;
    uint32_t lenBuffer;

    dataLen = recv(acceptSocket, &lenBuffer, 4, 0);
    if (dataLen == -1){
        throw std::runtime_error("Failed to recv message in dispatcher!");
    }

    if (dataLen == 0){
        // Socket has been disconnected
        throw SocketDisconnected();
    }

    auto messageSize = ntohl(lenBuffer);
    char messageBuffer[messageSize];
    memset(messageBuffer, '\0', messageSize);

    dataLen = 0;
    while (dataLen != messageSize) {
        auto recvLen = recv(acceptSocket,
                            messageBuffer + dataLen,
                            messageSize - dataLen,
                            0);

        if (recvLen == -1) {
            std::cerr << "Failed to recv()" << std::endl;
            throw std::runtime_error("Failed to recv from domain socket.");
        }

        dataLen += recvLen;
    }

    return {messageBuffer, messageSize};
}

json Dispatcher::processRequest(json &request) {
    json response;

    // Parse incoming request
    SocketFindRequest sockToFind;
    try{
        sockToFind = parseRequest(request);
    } catch (std::exception& e){
        return R"({"error":"wrong format"})"_json;
    }

    // Try to find the entry
    auto it = std::find(storage->socketHistory.begin(),
                        storage->socketHistory.end(),
                        sockToFind);
    if (it!= storage->socketHistory.end()){
        // send empty json as a sign of no data if no communications are available
        return it->requests.empty() ? json() :  it->getEntryAsJson();
    } else {
        return R"({"error":"not found"})"_json;
    }

}

void Dispatcher::sendResponse(int acceptSocket, json response) {
    auto responseString = response.dump();

    uint32_t messageSize = responseString.length();
    size_t payloadSize = responseString.length() + 4;

    char payload[payloadSize];
    memset(payload, '\0', payloadSize);

    auto nbo = htonl(messageSize);
    memcpy(payload, &nbo, 4);
    memcpy(payload+4, responseString.data(), messageSize);

    if (send(acceptSocket, payload, payloadSize, 0) < 0) {
        throw SendMessageFailed();
    }
}

SocketFindRequest Dispatcher::parseRequest(json &request) {
    std::string srcIp, dstIp;
    SocketFindRequest out;

    srcIp = request["srcIp"].get<std::string>();
    dstIp = request["dstIp"].get<std::string>();

    if (srcIp.find(':') != std::string::npos){
        out.ipVersion = 6;
        inet_pton(AF_INET6, srcIp.c_str(), &(out.src.ipv6));
        inet_pton(AF_INET6, dstIp.c_str(), &(out.dst.ipv6));
    } else {
        out.ipVersion = 4;
        inet_pton(AF_INET, srcIp.c_str(), &(out.src.ipv4.s_addr));
        inet_pton(AF_INET, dstIp.c_str(), &(out.dst.ipv4.s_addr));
    }

    out.srcPort = request["srcPort"].get<uint32_t>();
    out.dstPort = request["dstPort"].get<uint32_t>();
    out.ts = request["timestamp"].get<uint64_t>();

    return out;
}

