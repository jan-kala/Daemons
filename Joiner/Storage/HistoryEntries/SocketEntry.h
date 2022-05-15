//
// Created by Jan Kala on 09.05.2022.
//

#ifndef HTTPIPJOINER_SOCKETENTRY_H
#define HTTPIPJOINER_SOCKETENTRY_H

#include "ListEntryBase.h"
#include <list>
#include <arpa/inet.h>
#include <string>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace nlohmann;

class SocketBase {
public:
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
};

class SocketFindRequest : public SocketBase {
public:
    uint64_t ts;
};

class SocketEntry: public SocketBase, public ListEntryBase {
public:

    uint64_t ts_start = 0;
    uint64_t ts_end = 0;

    ListEntryBase* serverEntry;

    bool operator==(const SocketFindRequest &sock) const {
        bool isActive = ts_end == 0;
        bool inTsRange = ts_start <= sock.ts && (sock.ts <= ts_end || isActive);

        if (sock.ipVersion != ipVersion){
            return false;
        } else if (ipVersion == 4){
            return  inTsRange &&
                    sock.src.ipv4.s_addr == src.ipv4.s_addr &&
                    sock.srcPort == srcPort &&
                    sock.dst.ipv4.s_addr == dst.ipv4.s_addr &&
                    sock.dstPort == dstPort;

        } else if (ipVersion == 6){
            bool src_equal = std::memcmp(&(src.ipv6), &(sock.src.ipv6), 16) == 0;
            bool dst_equal = std::memcmp(&(dst.ipv6), &(sock.dst.ipv6), 16) == 0;

            return  inTsRange &&
                    src_equal &&
                    sock.srcPort == srcPort &&
                    dst_equal &&
                    sock.dstPort == dstPort;
        }
        return false;
    }

    json getEntryAsJson() override;
    void print(const char *outputFile) override;
    static SocketEntry proto2socketEntry(annotator::IFMessage &msg);
};


#endif //HTTPIPJOINER_SOCKETENTRY_H
