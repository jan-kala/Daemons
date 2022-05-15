//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"
#include "../Utils/LoggerCsv.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#define LOG_FILE_PATH "/Users/jan.kala/WebTrafficAnnotator/HttpReSender.csv"

int HttpDataReSender::run() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    connectSocket();

    while (true) {
        // Read json message from Browser Extension
        auto nativeMessage = getNativeMessage();
        auto protoMessage = json2protobuf(nativeMessage);

        // Send message to Joiner
        try {
            protoSend(protoMessage, &sockFd);
            LoggerCsv::log(protoMessage, LOG_FILE_PATH);
        } catch (SendMessageFailed& e){
            LoggerCsv::log(protoMessage, LOG_FILE_PATH, "FAILED_TO_SEND");
        }
    }
}

json
HttpDataReSender::getNativeMessage() {
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

annotator::HttpMessage
HttpDataReSender::json2protobuf(json &jsonMessage) {
    annotator::HttpMessage protoMessage;

    // Common part for all messages
    protoMessage.set_timestamp_eventtriggered(jsonMessage["timeStamp_s"].get<uint64_t>());
    protoMessage.set_data(jsonMessage["details"].dump());

    // Trigger-specific parts
    if (jsonMessage["trigger"].get<std::string>() == "onSendHeaders") {
        // Before anything is sent, this event is triggered

        protoMessage.set_type(annotator::HttpMessage_MessageType_NEW_REQUEST);
        protoMessage.set_hostname(jsonMessage["hostname"].get<std::string>());

        std::string proto = jsonMessage["proto"].get<std::string>();
        if (proto == "http:"){
            protoMessage.set_protocol(annotator::HttpMessage_RequestProtocol_HTTP);
        } else if (proto == "https:"){
            protoMessage.set_protocol(annotator::HttpMessage_RequestProtocol_TLS);
        } else {
            throw std::runtime_error("Wrong protocol");
        }

    } else if (jsonMessage["trigger"].get<std::string>() == "onResponseStarted"){
        // When response starts, connection is open and active, thus ready for pairing

        protoMessage.set_type(annotator::HttpMessage_MessageType_PAIRING);
        protoMessage.set_ipversion(jsonMessage["ipVersion"].get<int>());
        protoMessage.set_hostname(jsonMessage["hostname"].get<std::string>());
        protoMessage.set_serverport(jsonMessage["port"].get<int>());

        auto ipString = jsonMessage["ip"].get<std::string>();
        if (protoMessage.ipversion() == 4){
            in_addr ipv4{};
            inet_pton(AF_INET, ipString.c_str(), &(ipv4.s_addr));
            protoMessage.set_serveripv4(ipv4.s_addr);

        } else if (protoMessage.ipversion() == 6){
            in6_addr ipv6{};
            inet_pton(AF_INET6, ipString.c_str(), &(ipv6));
            protoMessage.set_serveripv6((const char *)&ipv6, 16);

        } else {
            throw std::runtime_error("Wrong IP version");
        }


        auto proto = jsonMessage["proto"].get<std::string>();
        if (proto == "http:"){
            protoMessage.set_protocol(annotator::HttpMessage_RequestProtocol_HTTP);
        } else if (proto == "https:"){
            protoMessage.set_protocol(annotator::HttpMessage_RequestProtocol_TLS);
        } else {
            throw std::runtime_error("Wrong protocol");
        }

    } else {
        protoMessage.set_type(annotator::HttpMessage_MessageType_DATA_ASSIGNMENT);
    }

    return protoMessage;
}
