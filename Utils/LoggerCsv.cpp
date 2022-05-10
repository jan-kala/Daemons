//
// Created by Jan Kala on 27.03.2022.
//

#include "LoggerCsv.h"
#include <arpa/inet.h>
#include <fstream>
#include <ostream>

void LoggerCsv::log(LoggerCsv::InterfaceInfo info) {
    std::string sep = ", ";
    std::cout<< info.timestamp << sep
             << "Interface" << sep
             << info.type << ","
             << "\"" << info.server_name << "\",\""
             << " src " << info.srcIP << " : " << info.srcPort << " |"
             << " dst " << info.dstIP << " : " << info.dstPort << "\""
             << std::endl;
}

void LoggerCsv::log(annotator::IFMessage &message, const char *outputPath) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);

    std::string sep = ", ";
    std::string timestamp =
            std::to_string(message.timestamp_s()) + "." + std::to_string(message.timestamp_ms());

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

    out<< message.timestamp_s() << "." << std::setfill('0') << std::setw(6) << message.timestamp_ms() << sep
             << "Interface" << sep
             << "\"" << message.servername()<< "\", "
             << msgType << ",\""
             << "src " << srcIP << " : " << message.srcport() << " |"
             << " dst " << dstIP << " : " << message.dstport() << "\""
             << std::endl;
}

void LoggerCsv::log(annotator::HttpMessage &message, const char *outputPath) {
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

    out << message.timestamp_s() << "."  << message.timestamp_ms()
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

