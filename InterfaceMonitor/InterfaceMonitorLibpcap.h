//
// Created by Jan Kala on 03.04.2022.
//

#ifndef INTERFACEMONITOR_INTERFACEMONITORLIBPCAP_H
#define INTERFACEMONITOR_INTERFACEMONITORLIBPCAP_H

#include "LoggerCsv.h"
#include <string>
#include <pcap.h>


class InterfaceMonitorLibpcap {
public:
    static int run();

    struct ParsedPacket {
        std::string srcIp;
        u_short srcPort;
        std::string dstIp;
        u_short dstPort;
        std::string tstamp;
        enum AppDataType {
            HTTP,
            TLS
        };
        std::string serverName;
        enum AppDataType type;
    };
private:
    static void onPacketArrives(u_char* cookie, const struct pcap_pkthdr* header, const u_char* packet);

    static LoggerCsv loggerCsv;
    static void logCsv(ParsedPacket& parsedPacket);
};


#endif //INTERFACEMONITOR_INTERFACEMONITORLIBPCAP_H
