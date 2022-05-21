//
// Created by Jan Kala on 18.04.2022.
//

#include "ProtobufSenderBase.h"
#include <iostream>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <fstream>
#include <arpa/inet.h>

ProtobufSenderBase::ProtobufSenderBase(Config& config): config(config) {
    this->domainSocketPath = config[CONFIG_KEY_DOMAIN_SOCKET_PATH].get<std::string>();
}

ProtobufSenderBase::~ProtobufSenderBase() {
    this->domainSocketPath = "";
    close(sockFd);
    sockFd = 0;
}

void ProtobufSenderBase::connectSocket() {
    int sock = 0;
    struct sockaddr_un remote;

    if ( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){
        std::cerr  << config.moduleName << ": Failed to create socket" << std::endl;
        return;
    }
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, domainSocketPath.c_str());
    int data_len = sizeof(remote);

    if (connect(sock, (struct sockaddr*)&remote, data_len) == -1){
        std::cerr << config.moduleName <<": Failed to connect to the socket." << std::endl;
        return;
    }

    this->sockFd = sock;
}

template<class T>
void ProtobufSenderBase::protoSend(T& message, int *socket) {
    uint32_t messageSize = message.ByteSizeLong();
    uint32_t payloadSize = messageSize + 4;

    u_char payload[payloadSize];
    memset(payload, '\0', payloadSize);

    // Save size into payload
    auto nbo = htonl(messageSize);
    memcpy(payload, &nbo, 4);

    // Save message into payload
    google::protobuf::io::ArrayOutputStream aos(payload+4, messageSize);
    google::protobuf::io::CodedOutputStream codedOutputStream(&aos);
    message.SerializeToCodedStream(&codedOutputStream);

    if (send(*socket, payload, payloadSize, 0) < 0){
        throw SendMessageFailed();
    }
}
template void
ProtobufSenderBase::protoSend(annotator::IFMessage& message, int *socket);
template void
ProtobufSenderBase::protoSend(annotator::HttpMessage& message, int *socket);
