//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"
#include "../Utils/LoggerCsv.h"

#define LOG_FILE_PATH "/tmp/WebAnnotator/browser.csv"

int HttpDataReSender::run() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    while (true) {
        json message = getNativeMessage()["message"];
        annotator::HttpMessage pbMessage = json2protobuf(message);
        LoggerCsv::log(pbMessage, LOG_FILE_PATH);
    }
    return 0;
}

json HttpDataReSender::getNativeMessage() {
    // Read length of the message first
    char raw_length[4];
    fread(raw_length, 4, sizeof(char), stdin);
    uint32_t msg_length = *reinterpret_cast<uint32_t*>(raw_length);
    if(!msg_length) {
        exit(0);
    }
    // Now read the whole message
    char message[msg_length];
    fread(message, msg_length, sizeof(char), stdin);
    std::string m(message, message + sizeof(message) / sizeof(message[0]));
    return json::parse(m);
}

annotator::HttpMessage HttpDataReSender::json2protobuf(json &message) {
    annotator::HttpMessage newMessage;
    newMessage.set_servername(message["url"].dump());

    return annotator::HttpMessage();
}
