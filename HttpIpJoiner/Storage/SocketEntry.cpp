//
// Created by Jan Kala on 09.05.2022.
//

#include "SocketEntry.h"

json
SocketEntry::getEntryAsJson(){
    std::string srcIP, dstIP;
    if (ipVersion == 4){
        char ipString[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(src.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
        srcIP = ipString;

        inet_ntop(AF_INET, &(dst.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
        dstIP = ipString;
    } else {
        char ipString[INET6_ADDRSTRLEN];

        inet_ntop(AF_INET6, &(src.ipv6), ipString, INET6_ADDRSTRLEN);
        srcIP = ipString;

        inet_ntop(AF_INET6, &(dst.ipv6), ipString, INET6_ADDRSTRLEN);
        dstIP = ipString;
    }
    return {
        {"start", ts_start},
        {"end", ts_end},
        {"ipVersion", ipVersion},
        {"srcIp", srcIP},
        {"srcPort", srcPort},
        {"dstIp", dstIP},
        {"dstPort", dstPort},
    };
}

void SocketEntry::print(){
    std::string srcIP, dstIP;
    if (ipVersion == 4){
        char ipString[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &(src.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
        srcIP = ipString;

        inet_ntop(AF_INET, &(dst.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
        dstIP = ipString;
    } else {
        char ipString[INET6_ADDRSTRLEN];

        inet_ntop(AF_INET6, &(src.ipv6), ipString, INET6_ADDRSTRLEN);
        srcIP = ipString;

        inet_ntop(AF_INET6, &(dst.ipv6), ipString, INET6_ADDRSTRLEN);
        dstIP = ipString;
    }
    std::cout
            << "  s-> "
            << "[" << ts_start << " - " << ts_end << "] "
            << "[" << srcIP << " " << srcPort << "] "
            << "[" << dstIP << " " << dstPort << "]"
            << std::endl;
}