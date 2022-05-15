//
// Created by Jan Kala on 27.03.2022.
//

#include "LoggerCsv.h"
#include <arpa/inet.h>
#include <fstream>
#include <ostream>
#include <sstream>
#include <iomanip>

void LoggerCsv::log(annotator::IFMessage &message, const char *outputPath,  const char *note) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);

    std::stringstream srcIP, dstIP;
    if (message.ipversion() == 4){
        char ipString[INET_ADDRSTRLEN];

        uint32_t ipV4 = message.srcv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        srcIP << std::setw(39) << ipString;

        ipV4 = message.dstv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        dstIP << std::setw(39) << ipString;
    } else {
        char ipString[INET6_ADDRSTRLEN];

        auto ipV6 = message.srcv6().data();
        inet_ntop(AF_INET6, ipV6, ipString, INET6_ADDRSTRLEN);
        srcIP << std::setw(39) << ipString;

        ipV6 = message.dstv6().data();
        inet_ntop(AF_INET6, ipV6, ipString, INET6_ADDRSTRLEN);
        dstIP << std::setw(39) << ipString;
    }

    std::string msgType;
    switch (message.type()) {
        case annotator::IFMessage_MessageType_TLS_NEW_CONNECTION:
            msgType = "ClientHello";
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN:
            msgType = "TCP FIN    ";
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST:
            msgType = "TCP RST    ";
            break;
        case annotator::IFMessage_MessageType_HTTP:
            msgType = "HTTP packet";
            break;
        case annotator::IFMessage_MessageType_UNWANTED_MESSAGE:
            msgType = "Ignored    ";
            break;
    }
    std::stringstream srcport, dstport;
    srcport << std::setw(5) << message.srcport();
    dstport << std::setw(5) << message.dstport();

    out << (note ? std::string(note) + ", " : "")
        << message.timestamp_packetcaptured() << ", "
        << "Interface, "
        << msgType << ", "
        << srcIP.str() << ", " << srcport.str() << ", "
        << dstIP.str() << ", " << dstport.str() << ", "
        << (!message.servername().empty() ? "\"" + message.servername() + "\"" : "\"\"")
        << std::endl;
    of.close();
}

void LoggerCsv::log(annotator::HttpMessage &message, const char *outputPath, const char *note) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);

    std::stringstream srcIP;
    if (message.ipversion() == 4){
        char ipString[INET_ADDRSTRLEN];

        uint32_t ipV4 = message.serveripv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        srcIP << std::setw(39) << ipString;
    } else {
        char ipString[INET6_ADDRSTRLEN];

        auto ipV6 = message.serveripv6().data();
        inet_ntop(AF_INET6, ipV6, ipString, INET6_ADDRSTRLEN);
        srcIP << std::setw(39) << ipString;
    }
    std::stringstream srcport, dstport;
    srcport << std::setw(5) << message.serverport();

    std::string msgType;
    switch (message.protocol()){
        case annotator::HttpMessage_RequestProtocol_HTTP:
            msgType = "data HTTP  ";
            break;
        case annotator::HttpMessage_RequestProtocol_TLS:
            msgType = "data HTTPS ";
            break;
        default:
            msgType = "";
            break;
    }

    out << (note ? std::string(note) + ", " : "")
        << message.timestamp_eventtriggered() << ", "
        << "Browser  , "
        << msgType << ", "
        << std::setw(39) << "" << ", " << std::setw(5) << "" << ", "
        << srcIP.str() << ", " << srcport.str() << ", "
        << "\"" << message.hostname() << "\", "
        << "\""  << message.data() << "\""
        << std::endl;
    of.close();
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

