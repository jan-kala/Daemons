//
// Created by Jan Kala on 27.03.2022.
//

#include "LoggerCsv.h"

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

