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
    char ipString[INET_ADDRSTRLEN];
    std::string srcIP, dstIP;
    if (message.ipversion() == 4){
        uint32_t ipV4 = message.srcv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        srcIP = ipString;

        ipV4 = message.dstv4();
        inet_ntop(AF_INET, &(ipV4), ipString, INET_ADDRSTRLEN);
        dstIP = ipString;
    }
    out<< timestamp << sep
             << "Interface" << sep
             << "\"" << message.servername() << "\",\""
             << " src " << srcIP << " : " << message.srcport() << " |"
             << " dst " << dstIP << " : " << message.dstport() << "\""
             << std::endl;
}

void LoggerCsv::log(annotator::HttpMessage &message, const char *outputPath) {
    std::ofstream of;
    std::ostream out = checkFileOutput(outputPath, of);

    out << "Hello" << std::endl;
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

