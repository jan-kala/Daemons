//
// Created by Jan Kala on 18.04.2022.
//

#include "ProtobufReceiverBase.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>

ProtobufReceiverBase::ProtobufReceiverBase(Config &config, const std::string& moduleName)
    : config(config)
    , sockFd(0)
    , acceptSockFd(0)
{
    this->domainSocketPath = config.common()[moduleName][CONFIG_KEY_DOMAIN_SOCKET_PATH].get<std::string>();
}

ProtobufReceiverBase::~ProtobufReceiverBase() {
    this->domainSocketPath = "";
    close(sockFd);
    close(acceptSockFd);
    sockFd = 0;
    acceptSockFd = 0;
}

void ProtobufReceiverBase::connectSocket() {
    int sock = 0;
    struct sockaddr_un local, remote;

    if ( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){
        std::cerr << config.moduleName << ": Failed to create socket" << std::endl;
        return;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, domainSocketPath.c_str());
    int data_len = sizeof(local);

    unlink(domainSocketPath.c_str());

    if (bind(sock, (struct sockaddr*)&local, data_len) == -1){
        std::cerr << config.moduleName << ": Failed to bind to the socket." << std::endl;
        close(sock);
        return;
    }

    if (listen(sock, 100) != 0) {
        std::cerr << config.moduleName << ": Failed to listen()" << std::endl;
        close(sock);
        return;
    }

    int accept_socket = 0;
    u_int sock_len = 0;
    accept_socket = accept(sock, (struct sockaddr*)&remote, &sock_len);
    if ( accept_socket == -1){
        std::cerr<< config.moduleName << ": Failed to accept()" << std::endl;
        return;
    }
    this->sockFd = sock;
    this->acceptSockFd = accept_socket;
}

template<class T>
T ProtobufReceiverBase::recvMessage() {
    int dataLen = 0;
    uint32_t lengthBuffer;

    dataLen = recv(this->acceptSockFd, &lengthBuffer, 4, 0);
    if ( dataLen == -1) {
        throw std::runtime_error("Failed to read length of the message");
    }
    if (dataLen == 0){
        // Socket has been disconnected
        throw SocketDisconnected();
    }

    // Read size of the message from first 4 bytes
    auto messageSize = ntohl(lengthBuffer);

    // READ rest of the body
    u_char payload[messageSize];

    dataLen = 0;
    while (dataLen != messageSize) {
        auto recvLen = recv(this->acceptSockFd,
                                payload + dataLen,
                                messageSize - dataLen,
                                0);

        if (recvLen == -1) {
            std::cerr << "Failed to recv()" << std::endl;
            throw std::runtime_error("Failed to recv from domain socket.");
        }

        dataLen += recvLen;
    }

    google::protobuf::io::ArrayInputStream ais2(payload, messageSize);
    google::protobuf::io::CodedInputStream codedInputStream2(&ais2);
    google::protobuf::io::CodedInputStream::Limit msgLimit = codedInputStream2.PushLimit(messageSize);
    T message;
    message.ParseFromCodedStream(&codedInputStream2);
    codedInputStream2.PopLimit(msgLimit);

    return message;
}

template annotator::IFMessage
ProtobufReceiverBase::recvMessage<annotator::IFMessage>();
template annotator::HttpMessage
ProtobufReceiverBase::recvMessage<annotator::HttpMessage>();
