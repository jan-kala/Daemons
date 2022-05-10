//
// Created by Jan Kala on 09.05.2022.
//

#ifndef HTTPIPJOINER_SOCKETCONNECTIONLIST_H
#define HTTPIPJOINER_SOCKETCONNECTIONLIST_H

#include <list>
#include <arpa/inet.h>
#include <string>
#include <iostream>

class SocketConnectionList {
public:
    struct SocketEntry {
        uint16_t ipVersion;
        union {
            struct in_addr ipv4;
            struct in6_addr ipv6;
        } src;
        uint32_t srcPort;
        union {
            struct in_addr ipv4;
            struct in6_addr ipv6;
        } dst;
        uint32_t dstPort;

        uint64_t ts_start;
        uint64_t ts_end = 69;
    };

    static void print(SocketEntry *entry){
        std::string srcIP, dstIP;
        if (entry->ipVersion == 4){
            char ipString[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &(entry->src.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
            srcIP = ipString;

            inet_ntop(AF_INET, &(entry->dst.ipv4.s_addr), ipString, INET_ADDRSTRLEN);
            dstIP = ipString;
        } else {
            char ipString[INET6_ADDRSTRLEN];

            inet_ntop(AF_INET6, &(entry->src.ipv6), ipString, INET6_ADDRSTRLEN);
            srcIP = ipString;

            inet_ntop(AF_INET6, &(entry->dst.ipv6), ipString, INET6_ADDRSTRLEN);
            dstIP = ipString;
        }
        std::cout << "  s-> " << srcIP << ":" << entry->srcPort << "\t"
                              << dstIP << ":" << entry->dstPort << "\t"
                              << "[" << entry->ts_start << " - " << entry->ts_end << "]" << std::endl;
    }

    std::list<SocketEntry> connections;
};


#endif //HTTPIPJOINER_SOCKETCONNECTIONLIST_H
