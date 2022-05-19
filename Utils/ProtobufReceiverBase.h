//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_PROTOBUFRECEIVERBASE_H
#define HTTPIPJOINER_PROTOBUFRECEIVERBASE_H

#include <string>
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "Config.h"

class ProtobufReceiverBase {
public:
    explicit ProtobufReceiverBase(Config& config, const std::string& moduleName);
    ~ProtobufReceiverBase();

protected:
    class SocketDisconnected : std::exception{
        [[nodiscard]] const char* what() const noexcept override{
            return "Socket has been disconnected";
        }
    };

    Config& config;
    std::string domainSocketPath;
    int sockFd;
    int acceptSockFd;

    void connectSocket();

    template<class T> T recvMessage();
};


#endif //HTTPIPJOINER_PROTOBUFRECEIVERBASE_H
