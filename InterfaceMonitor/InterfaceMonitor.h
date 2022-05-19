//
// Created by Jan Kala on 03.04.2022.
//

#ifndef INTERFACEMONITOR_INTERFACEMONITOR_H
#define INTERFACEMONITOR_INTERFACEMONITOR_H

#include "../Utils/LoggerCsv.h"
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../Utils/ProtobufSenderBase.h"
#include "../../Utils/Config.h"
#include <string>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>

class InterfaceMonitor: ProtobufSenderBase{
public:
    explicit InterfaceMonitor(Config& config)
        : ProtobufSenderBase(config)
        , logger(config)
        , config(config) {};

    int run();

    static void
    onPacketArrives(u_char* cookie, const struct pcap_pkthdr* header, const u_char* packet);

    static u_char *
    parseEthernetIpTcpHeaders(const u_char *packet, annotator::IFMessage &message);

    static uint16_t
    parseIpV4Header(const ip *ipPayload, annotator::IFMessage &message);

    static uint16_t
    parseIpV6Header(const ip6_hdr *ipPayload, annotator::IFMessage &message);

    static uint16_t
    parseTcpHeader(const struct tcphdr *tcpPayload, annotator::IFMessage &message);

    static bool
    isHttp(const u_char* payload);

    static void
    parseHTTPPayload(std::string &payload, annotator::IFMessage &message);

    static bool
    parseTlsPayload(u_char* payload, annotator::IFMessage &message);

    static char*
    getTlsSni(u_char *bytes, int* len);

    Config& config;
    LoggerCsv logger;
};


#endif //INTERFACEMONITOR_INTERFACEMONITOR_H
