//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPDATARESENDER_PROTOBUFSENDERBASE_H
#define HTTPDATARESENDER_PROTOBUFSENDERBASE_H

#include <string>

class ProtobufSenderBase {
public:
    explicit ProtobufSenderBase(std::string& domainSocketPath);
    ~ProtobufSenderBase();

protected:
    std::string domainSocketPath;
    int sockFd = 0;
    void connectSocket();
};


#endif //HTTPDATARESENDER_PROTOBUFSENDERBASE_H
