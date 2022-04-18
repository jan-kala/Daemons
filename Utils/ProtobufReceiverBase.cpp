//
// Created by Jan Kala on 18.04.2022.
//

#include "ProtobufReceiverBase.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>

ProtobufReceiverBase::ProtobufReceiverBase(std::string &domainSocketPath) {
    this->domainSocketPath = domainSocketPath;
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
        std::cerr << "Joiner: Failed to create socket" << std::endl;
        return;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, domainSocketPath.c_str());
    int data_len = sizeof(local);

    unlink(domainSocketPath.c_str());

    if (bind(sock, (struct sockaddr*)&local, data_len) == -1){
        std::cerr << "Joiner: Failed to bind to the socket." << std::endl;
        close(sock);
        return;
    }

    if (listen(sock, 100) != 0) {
        std::cerr << "Joiner: Failed to listen()" << std::endl;
        close(sock);
        return;
    }

    int accept_socket = 0;
    u_int sock_len = 0;
    accept_socket = accept(sock, (struct sockaddr*)&remote, &sock_len);
    if ( accept_socket == -1){
        std::cerr<< "Joiner: Failed to accept()" << std::endl;
        return;
    }
    this->sockFd = sock;
    this->acceptSockFd = accept_socket;
}

