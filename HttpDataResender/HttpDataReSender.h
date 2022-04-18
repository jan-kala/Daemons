//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPDATARESENDER_HTTPDATARESENDER_H
#define HTTPDATARESENDER_HTTPDATARESENDER_H

#include "../Utils/ProtobufSenderBase.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include <nlohmann/json.hpp>

using namespace nlohmann;

class HttpDataReSender: ProtobufSenderBase {
public:

    explicit HttpDataReSender(std::string& domainSocketPath)
        : ProtobufSenderBase(domainSocketPath){};

    int run();

private:
    json getNativeMessage();
    annotator::HttpMessage json2protobuf(json& message);
};


#endif //HTTPDATARESENDER_HTTPDATARESENDER_H
