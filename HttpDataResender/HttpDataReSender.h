//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPDATARESENDER_HTTPDATARESENDER_H
#define HTTPDATARESENDER_HTTPDATARESENDER_H

#include "../Utils/ProtobufSenderBase.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include <nlohmann/json.hpp>
#include "../../Utils/Config.h"
#include "../../Utils/LoggerCsv.h"

using namespace nlohmann;

class HttpDataReSender: ProtobufSenderBase {
public:

    explicit HttpDataReSender(Config& config);

    int run();

private:
    LoggerCsv logger;
    Config& config;

    json getNativeMessage();
    annotator::HttpMessage json2protobuf(json& jsonMessage);
};


#endif //HTTPDATARESENDER_HTTPDATARESENDER_H
