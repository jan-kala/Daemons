//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"
#include "../Utils/LoggerCsv.h"
#include <sys/socket.h>

#define LOG_FILE_PATH "/tmp/WebAnnotator/browser.csv"

int HttpDataReSender::run() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    connectSocket();

    //testing message
//    annotator::HttpMessage testmsg;
//    std::string sni = "test.com/testing/test";
//    testmsg.set_servername(sni);
//    testmsg.set_timestamp_s(1647894753785);
//    testmsg.set_timestamp_ms(1812);
//    std::string data = "Toto jsou data ktera posilam protobufem.";
//    testmsg.set_data(data);
//
//    LoggerCsv::log(testmsg);
//    protoSend(testmsg);

    while (true) {
        json message = getNativeMessage()["message"];
        annotator::HttpMessage pbMessage = json2protobuf(message);
        protoSend(pbMessage);
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

    float timestamp = std::stof(message["timeStamp"].dump());
    timestamp = timestamp / 1000;
    float ts_s, ts_ms;
    ts_ms = std::modf(timestamp, &ts_s);

    newMessage.set_timestamp_s(static_cast<int>(ts_s));
    newMessage.set_timestamp_ms(static_cast<int>(ts_ms*1000));

    newMessage.set_data(message.dump());

    return newMessage;
}

void HttpDataReSender::protoSend(annotator::HttpMessage &message) {
    size_t payloadSize = message.ByteSizeLong() + 4;
    char payload[payloadSize];
    memset(payload, '\0', payloadSize);
    google::protobuf::io::ArrayOutputStream aos(payload, payloadSize);
    google::protobuf::io::CodedOutputStream codedOutputStream(&aos);

    codedOutputStream.WriteVarint32(message.ByteSizeLong());
    message.SerializeToCodedStream(&codedOutputStream);

    if (send(sockFd, payload, payloadSize, 0) < 0){
        std::cerr << "HttpDataReSender: Failed to send data to Joiner!" << std::endl;
    }
}
