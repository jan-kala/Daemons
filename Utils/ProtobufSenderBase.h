//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPDATARESENDER_PROTOBUFSENDERBASE_H
#define HTTPDATARESENDER_PROTOBUFSENDERBASE_H

#include <string>
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"

class ProtobufSenderBase {
public:
    explicit ProtobufSenderBase(std::string& domainSocketPath);
    ~ProtobufSenderBase();

protected:
    class SendMessageFailed : std::exception{
        [[nodiscard]] const char* what() const noexcept override{
            return "Failed to send Protobuf message";
        }
    };

    std::string domainSocketPath;
    int sockFd = 0;
    void connectSocket();

    template<class T> static void protoSend(T& message, int *socket);
};


#endif //HTTPDATARESENDER_PROTOBUFSENDERBASE_H
