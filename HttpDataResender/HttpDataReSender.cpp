//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"
#include "../Utils/LoggerCsv.h"
#include <sys/socket.h>
#include <arpa/inet.h>

#define LOG_FILE_PATH "/tmp/WebAnnotator/browser.csv"

int HttpDataReSender::run() {
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    connectSocket();

    while (true) {
        json message = getNativeMessage();
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

    // Common part for all messages
    newMessage.set_timestamp_s(message["timeStamp_s"].get<uint64_t>());
    newMessage.set_timestamp_ms(message["timeStamp_ms"].get<int>());

    newMessage.set_data(message["details"].dump());

    // Trigger-specific parts
    if (message["trigger"].get<std::string>() == "onSendHeaders") {
        newMessage.set_type(annotator::HttpMessage_MessageType_NEW_REQUEST);
        newMessage.set_hostname(message["hostname"].get<std::string>());

        std::string proto = message["proto"].get<std::string>();
        if (proto == "http:"){
            newMessage.set_protocol(annotator::HttpMessage_RequestProtocol_HTTP);
        } else if (proto == "https:"){
            newMessage.set_protocol(annotator::HttpMessage_RequestProtocol_TLS);
        } else {
            throw std::runtime_error("Wrong protocol");
        }

    } else if (message["trigger"].get<std::string>() == "onResponseStarted"){
        // When response starts, connection is open and active, thus ready for pairing

        newMessage.set_type(annotator::HttpMessage_MessageType_PAIRING);

        newMessage.set_ipversion(message["ipVersion"].get<int>());
        newMessage.set_hostname(message["hostname"].get<std::string>());
        newMessage.set_serverport(message["port"].get<int>());

        std::string ipString = message["ip"].get<std::string>();
        if (newMessage.ipversion() == 4){
            in_addr ipv4;
            inet_pton(AF_INET, ipString.c_str(), &(ipv4.s_addr));
            newMessage.set_serveripv4(ipv4.s_addr);

        } else if (newMessage.ipversion() == 6){
            in6_addr ipv6;
            inet_pton(AF_INET6, ipString.c_str(), &(ipv6));
            newMessage.set_serveripv6((const char *)&ipv6, 16);

        } else {
            throw std::runtime_error("Wrong IP version");
        }


        std::string proto = message["proto"].get<std::string>();
        if (proto == "http:"){
            newMessage.set_protocol(annotator::HttpMessage_RequestProtocol_HTTP);
        } else if (proto == "https:"){
            newMessage.set_protocol(annotator::HttpMessage_RequestProtocol_TLS);
        } else {
            throw std::runtime_error("Wrong protocol");
        }

    } else {
        newMessage.set_type(annotator::HttpMessage_MessageType_DATA_ASSIGNMENT);
    }

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
