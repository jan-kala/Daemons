//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_PROTOBUFRECEIVERBASE_H
#define HTTPIPJOINER_PROTOBUFRECEIVERBASE_H

#include <string>

class ProtobufReceiverBase {
public:
    explicit ProtobufReceiverBase(std::string& domainSocketPath);
    ~ProtobufReceiverBase();

protected:
    std::string domainSocketPath;
    int sockFd;
    int acceptSockFd;
    void connectSocket();
};


#endif //HTTPIPJOINER_PROTOBUFRECEIVERBASE_H
