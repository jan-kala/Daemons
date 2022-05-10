//
// Created by Jan Kala on 18.04.2022.
//

#include "ProtobufSenderBase.h"
#include <iostream>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

ProtobufSenderBase::ProtobufSenderBase(std::string& domainSocketPath) {
    this->domainSocketPath = domainSocketPath;
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
        std::cerr << "IFMonitor: Failed to create socket" << std::endl;
        return;
    }
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, domainSocketPath.c_str());
    int data_len = sizeof(remote);

    if (connect(sock, (struct sockaddr*)&remote, data_len) == -1){
        std::cerr << "IFMonitor: Failed to connect to the socket." << std::endl;
        return;
    }

    this->sockFd = sock;
}

template<class T>
void ProtobufSenderBase::protoSend(T& message, int *socket) {
    size_t payloadSize = message.ByteSizeLong() + 4;
    char payload[payloadSize];
    memset(payload, '\0', payloadSize);
    google::protobuf::io::ArrayOutputStream aos(payload, payloadSize);
    google::protobuf::io::CodedOutputStream codedOutputStream(&aos);

    codedOutputStream.WriteVarint32(message.ByteSizeLong());
    message.SerializeToCodedStream(&codedOutputStream);

    if (send(*socket, payload, payloadSize, 0) < 0){
        throw SendMessageFailed();
    }
}
template void
ProtobufSenderBase::protoSend(annotator::IFMessage& message, int *socket);
template void
ProtobufSenderBase::protoSend(annotator::HttpMessage& message, int *socket);
