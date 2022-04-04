//
// Created by Jan Kala on 03.04.2022.
//

#include "InterfaceMonitorLibpcap.h"
#include "PacketDefinitions.h"
#include <iostream>
#include <pcap.h>

#define IFMONITOR_RETURN_SUCCESS 0
#define IFMONITOR_RETURN_ERROR 1

/**
 * BPF Filter for capturing.
 *
 * First is TLS filter for capturing Client Hello messages,
 * that containst SNI.
 * Filter is taken from https://gist.github.com/LeeBrotherston/92cc2637f33468485b8f
 * Last line of filter is HTTP GET request filter.
 */
#define PACKET_CAPTURE_FILTER \
    "(tcp[tcp[12]/16*4]=22 and (tcp[tcp[12]/16*4+5]=1) and (tcp[tcp[12]/16*4+9]=3) and (tcp[tcp[12]/16*4+1]=3))" \
    "or " \
    "(ip6[(ip6[52]/16*4)+40]=22 and (ip6[(ip6[52]/16*4+5)+40]=1) and (ip6[(ip6[52]/16*4+9)+40]=3) and (ip6[(ip6[52]/16*4+1)+40]=3))" \
    "or " \
    "((udp[14] = 6 and udp[16] = 32 and udp[17] = 1) and ((udp[(udp[60]/16*4)+48]=22) and (udp[(udp[60]/16*4)+53]=1) and (udp[(udp[60]/16*4)+57]=3) and (udp[(udp[60]/16*4)+49]=3))) " \
    "or " \
    "(proto 41 and ip[26] = 6 and ip[(ip[72]/16*4)+60]=22 and (ip[(ip[72]/16*4+5)+60]=1) and (ip[(ip[72]/16*4+9)+60]=3) and (ip[(ip[72]/16*4+1)+60]=3))"                               \
    "or"                            \
    "(tcp and tcp[32:4] =  0x47455420)"

int InterfaceMonitorLibpcap::run() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* devList;
    if (0 != pcap_findalldevs(&devList, errbuf)){
        std::cerr << "Failed to find devices with error: " << errbuf << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }

    pcap_t* handle = pcap_open_live(devList[0].name, BUFSIZ, 0, 1000, errbuf);
    if (handle == nullptr){
        std::cerr << "Failed to open device with error: " << errbuf << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }

    struct bpf_program filter = {};
    if (-1 == pcap_compile(handle, &filter, PACKET_CAPTURE_FILTER, 0, PCAP_NETMASK_UNKNOWN)){
        std::cerr << "Failed to compile BPF filter for TLS with error: " << pcap_geterr(handle) << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }
    if (-1 == pcap_setfilter(handle, &filter)){
        std::cerr << "Failed to set BPF filter for TLS with error: " << pcap_geterr(handle) << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }

    pcap_loop(handle, -1, onPacketArrives, nullptr);


    return IFMONITOR_RETURN_SUCCESS;
}

char *get_TLS_SNI(unsigned char *bytes, int* len)
{
    unsigned char *curr;
    unsigned char sidlen = bytes[43];
    curr = bytes + 1 + 43 + sidlen;
    unsigned short cslen = ntohs(*(unsigned short*)curr);
    curr += 2 + cslen;
    unsigned char cmplen = *curr;
    curr += 1 + cmplen;
    unsigned char *maxchar = curr + 2 + ntohs(*(unsigned short*)curr);
    curr += 2;
    unsigned short ext_type = 1;
    unsigned short ext_len;
    while(curr < maxchar && ext_type != 0)
    {
        ext_type = ntohs(*(unsigned short*)curr);
        curr += 2;
        ext_len = ntohs(*(unsigned short*)curr);
        curr += 2;
        if(ext_type == 0)
        {
            curr += 3;
            unsigned short namelen = ntohs(*(unsigned short*)curr);
            curr += 2;
            *len = namelen;
            return (char*)curr;
        }
        else curr += ext_len;
    }
    if (curr != maxchar) return nullptr;
    return NULL; //SNI was not present
}

void InterfaceMonitorLibpcap::onPacketArrives(u_char *cookie, const struct pcap_pkthdr *header, const u_char *packet)
{
    ParsedPacket parsedPacket;
    parsedPacket.tstamp = std::to_string(header->ts.tv_sec) + "." + std::to_string(header->ts.tv_usec);

    const struct sniff_ethernet *ethernet; /* The ethernet header */
    const struct sniff_ip *ip; /* The IP header */
    const struct sniff_tcp *tcp; /* The TCP header */
    const u_char *payload; /* Packet payload */

    ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);
    u_int sizeIp = IP_HL(ip) * 4;
    if (sizeIp < 20){
        std::cerr << "Invalid IP header size: " << sizeIp << std::endl;
        return;
    }
    char ipString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ip->ip_src.s_addr), ipString, INET_ADDRSTRLEN);
    parsedPacket.srcIp = ipString;
    inet_ntop(AF_INET, &(ip->ip_dst.s_addr), ipString, INET_ADDRSTRLEN);
    parsedPacket.dstIp = ipString;

    tcp = (struct sniff_tcp*) (packet + SIZE_ETHERNET + sizeIp);
    u_int sizeTcp = TH_OFF(tcp)*4;
    if (sizeTcp < 20) {
        std::cerr << "Invalid TCP header size: " << TH_OFF(tcp) << std::endl;
        return;
    }

    parsedPacket.srcPort = ntohs(tcp->th_sport);
    parsedPacket.dstPort = ntohs(tcp->th_dport);

    payload = (u_char *)(packet + SIZE_ETHERNET + sizeIp + sizeTcp);

    if ( payload[0] == 'G' && payload[1] == 'E' && payload[2] == 'T') {
        parsedPacket.type = ParsedPacket::AppDataType::HTTP;
        auto urlStart = (char*)(&payload[4]);
        auto urlEnd = strchr((char*)urlStart, ' ');
        parsedPacket.serverName = std::string(urlStart, urlEnd);
    } else {
        parsedPacket.type = ParsedPacket::AppDataType::TLS;
        int sniLen;
        char* sniStart = get_TLS_SNI((u_char*)payload, &sniLen);
        parsedPacket.serverName = std::string(sniStart, sniLen);
    }
    logCsv(parsedPacket);

}

void InterfaceMonitorLibpcap::logCsv(ParsedPacket& parsedPacket) {

    LoggerCsv::InterfaceInfo info;
    info.srcIP = parsedPacket.srcIp;
    info.srcPort = std::to_string(parsedPacket.srcPort);
    info.dstIP = parsedPacket.dstIp;
    info.dstPort = std::to_string(parsedPacket.dstPort);
    info.server_name = parsedPacket.serverName;
    info.timestamp = parsedPacket.tstamp;
    info.type = parsedPacket.type == ParsedPacket::AppDataType::HTTP ? "HTTP" : "TLS";

    loggerCsv.log(info);
}

