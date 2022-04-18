//
// Created by Jan Kala on 03.04.2022.
//

#ifndef INTERFACEMONITOR_INTERFACEMONITOR_H
#define INTERFACEMONITOR_INTERFACEMONITOR_H

#include "../Utils/LoggerCsv.h"
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../Utils/ProtobufSenderBase.h"
#include <string>
#include <pcap.h>


class InterfaceMonitor: ProtobufSenderBase{
public:
    explicit InterfaceMonitor(std::string& domainSocketPath)
        : ProtobufSenderBase(domainSocketPath){};

    int run();

private:
    static void onPacketArrives(u_char* cookie, const struct pcap_pkthdr* header, const u_char* packet);
    static u_char *parseEthernetIpTcpHeaders(const u_char *packet, annotator::IFMessage &message);
    static bool isHttp(const u_char* payload);
    static void parseHTTPPayload(std::string &payload, annotator::IFMessage &message);
    static void parseTlsPayload(u_char* payload, annotator::IFMessage &message);
    static char* getTlsSni(u_char *bytes, int* len);

    static void protoSend(annotator::IFMessage &message, int *socket);
};


#endif //INTERFACEMONITOR_INTERFACEMONITOR_H
