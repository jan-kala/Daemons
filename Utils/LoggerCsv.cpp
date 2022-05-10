//
// Created by Jan Kala on 27.03.2022.
//

#include "LoggerCsv.h"
#include <arpa/inet.h>
#include <fstream>
#include <ostream>

void LoggerCsv::log(annotator::IFMessage &message, const char *outputPath,  const char *note) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);
    std::string sep = ", ";

    std::string srcIP, dstIP;
    if (message.ipversion() == 4){
        char ipString[INET_ADDRSTRLEN];

        uint32_t ipV4 = message.srcv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        srcIP = ipString;

        ipV4 = message.dstv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        dstIP = ipString;
    } else {
        char ipString[INET6_ADDRSTRLEN];

        auto ipV6 = message.srcv6().data();
        inet_ntop(AF_INET6, ipV6, ipString, INET6_ADDRSTRLEN);
        srcIP = ipString;

        ipV6 = message.dstv6().data();
        inet_ntop(AF_INET6, ipV6, ipString, INET6_ADDRSTRLEN);
        dstIP = ipString;
    }
    std::string msgType;
    switch (message.type()) {
        case annotator::IFMessage_MessageType_TLS_NEW_CONNECTION:
            msgType = "NEW_CONNECTION";
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN:
            msgType = "FIN";
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST:
            msgType = "RST";
            break;
        case annotator::IFMessage_MessageType_HTTP:
            msgType = "HTTP";
            break;
        case annotator::IFMessage_MessageType_UNWANTED_MESSAGE:
            msgType = "UNWANTED";
            break;
    }

    out << (note ? note : "")
        << message.timestamp_packetcaptured() << sep
        << "Interface" << sep
        << "\"" << message.servername()<< "\", "
        << msgType << ",\""
        << "src " << srcIP << " : " << message.srcport() << " |"
        << " dst " << dstIP << " : " << message.dstport() << "\""
        << std::endl;
}

void LoggerCsv::log(annotator::HttpMessage &message, const char *outputPath, const char *note) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);

    std::string msgType;
    switch (message.type()){
        case annotator::HttpMessage_MessageType_NEW_REQUEST:
            msgType = "NEW_REQUEST";
            break;
        case annotator::HttpMessage_MessageType_PAIRING:
            msgType = "PAIRING";
            break;
        default:
            msgType = "";
            break;
    }

    out << (note ? note : "")
        << message.timestamp_eventtriggered()
        << ", Browser, " << msgType
        << "," << message.hostname() << ",\""
        << message.data() << "\""
        << std::endl;
}

std::ostream LoggerCsv::checkFileOutput(const char *filePath, std::ofstream &of) {
    std::streambuf* buf;

    if (filePath){
        of.open(filePath, std::ofstream::app);
        buf = of.rdbuf();
    } else {
        buf = std::cout.rdbuf();
    }
    return std::ostream(buf);
}

