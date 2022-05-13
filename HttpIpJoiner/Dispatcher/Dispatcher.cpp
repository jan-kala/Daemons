//
// Created by Jan Kala on 13.05.2022.
//

#include "Dispatcher.h"

Dispatcher::Dispatcher(int port, ActiveConnectionsPool *pool)
    : pool(pool)
    , port(port)
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
        throw std::runtime_error("Failed to create dispatcher socket!");
    }

    int opt = 1;
    if (setsockopt(newSockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        throw std::runtime_error("Failed to set socket options!");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(newSockFd, (struct sockaddr*)&addr, sizeof(addr)) < 0){
        throw std::runtime_error("Filed to bind socket!");
    }

    sockFd = newSockFd;
}

void Dispatcher::dispatchRequest() {

    if (listen(sockFd, 100) < 0) {
        throw std::runtime_error("Failed to listen for requests!");
    }

    struct sockaddr_in addr = {};
    int addrLen = sizeof(addr);
    auto acceptSocket = accept(sockFd, (struct sockaddr*)&addr, (socklen_t*)&addrLen);
    if (acceptSocket < 0){
        throw std::runtime_error("Failed to create communication socket!");
    }

    std::string strMessage;
    try {
        strMessage = readRequest(acceptSocket);
    } catch (SocketDisconnected& e){
        std::cerr<<"Request hasn't been dispatched, client disconnected." << std::endl;
        return;
    }
    json jsonMessage = json::parse(strMessage);
    std::cout<< jsonMessage.dump(2) << std::endl;

    auto response = processRequest(jsonMessage);
    sendResponse(acceptSocket, response);

}

std::string Dispatcher::readRequest(int acceptSocket) {
    size_t dataLen = 0;
    u_char lenBuffer[4] ;


    dataLen = recv(acceptSocket, lenBuffer, 4, 0);
    if (dataLen == -1){
        throw std::runtime_error("Failed to recv message in dispatcher!");
    }

    if (dataLen == 0){
        // Socket has been disconnected
        throw SocketDisconnected();
    }

    auto messageSize = (uint32_t)*lenBuffer;
    char messageBuffer[messageSize];
    dataLen = recv(acceptSocket, messageBuffer, messageSize, 0);

    return std::string(messageBuffer);
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
    auto it = std::find(pool->socketHistory.begin(),
                        pool->socketHistory.end(),
                        sockToFind);
    if (it!= pool->socketHistory.end()){
        return it->serverEntry->getEntryAsJson();
    } else {
        return R"({"error":"not found"})"_json;
    }

}

void Dispatcher::sendResponse(int acceptSocket, json response) {
    auto responseString = response.dump();
    size_t payloadSize = responseString.length() + 4;
    char payload[payloadSize];
    memset(payload, '\0', payloadSize);

    memcpy(payload, (char *)&payloadSize, 4);
    memcpy(payload+4, responseString.data(), responseString.length());

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

