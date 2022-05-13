//
// Created by Jan Kala on 03.04.2022.
//

#include "InterfaceMonitor.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <sys/un.h>

#define IFMONITOR_RETURN_SUCCESS 0
#define IFMONITOR_RETURN_ERROR 1

/**
 * BPF Filter for capturing.
 *
 * Contains TLS filter for capturing Client Hello messages,
 * that has SNI set.
 * Filter is taken from https://gist.github.com/LeeBrotherston/92cc2637f33468485b8f
 *
 * ipv4 tcp FIN is set
 * ipv6 tcp FIN is set
 * ipv4 tcp RST is set
 * ipv6 tcp RST is set
 * TLS filter taken from the source
 * HTTP filter (looks for GET in message -> GET = 0x47455420)
 */
#define PACKET_CAPTURE_FILTER \
    "((tcp[tcpflags] & tcp-fin) != 0) " \
    "or "                     \
    "(tcp and ip6[13+40]&0x01 != 0) " \
    "or "                     \
    "((tcp[tcpflags] & tcp-rst) != 0) " \
    "or "                     \
    "(tcp and ip6[13+40]&0x04 != 0) " \
    "or "                     \
    "(tcp[tcp[12]/16*4]=22 and (tcp[tcp[12]/16*4+5]=1) and (tcp[tcp[12]/16*4+9]=3) and (tcp[tcp[12]/16*4+1]=3)) " \
    "or " \
    "(ip6[(ip6[52]/16*4)+40]=22 and (ip6[(ip6[52]/16*4+5)+40]=1) and (ip6[(ip6[52]/16*4+9)+40]=3) and (ip6[(ip6[52]/16*4+1)+40]=3)) " \
    "or " \
    "((udp[14] = 6 and udp[16] = 32 and udp[17] = 1) and ((udp[(udp[60]/16*4)+48]=22) and (udp[(udp[60]/16*4)+53]=1) and (udp[(udp[60]/16*4)+57]=3) and (udp[(udp[60]/16*4)+49]=3))) " \
    "or " \
    "(proto 41 and ip[26] = 6 and ip[(ip[72]/16*4)+60]=22 and (ip[(ip[72]/16*4+5)+60]=1) and (ip[(ip[72]/16*4+9)+60]=3) and (ip[(ip[72]/16*4+1)+60]=3)) " \
    "or "  \
    "(tcp and tcp[32:4] =  0x47455420) "

int
InterfaceMonitor::run() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t* devList;
    if (0 != pcap_findalldevs(&devList, errbuf)){
        std::cerr << "Failed to find devices with error: " << errbuf << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }

//    pcap_t* handle = pcap_open_live(devList[0].name, BUFSIZ, 0, 1000, errbuf);
//    if (handle == nullptr){
//        std::cerr << "Failed to open device with error: " << errbuf << std::endl;
//        return IFMONITOR_RETURN_ERROR;
//    }

    pcap_t* handle = pcap_create(devList[0].name, errbuf);
    pcap_set_immediate_mode(handle, 1);
    pcap_activate(handle);


    struct bpf_program filter = {};
    if (-1 == pcap_compile(handle, &filter, PACKET_CAPTURE_FILTER, 0, PCAP_NETMASK_UNKNOWN)){
        std::cerr << "Failed to compile BPF filter for TLS with error: " << pcap_geterr(handle) << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }
    if (-1 == pcap_setfilter(handle, &filter)){
        std::cerr << "Failed to set BPF filter for TLS with error: " << pcap_geterr(handle) << std::endl;
        return IFMONITOR_RETURN_ERROR;
    }

    connectSocket();

    pcap_loop(handle, -1, onPacketArrives, (u_char*)this);

    return IFMONITOR_RETURN_SUCCESS;
}

void
InterfaceMonitor::onPacketArrives(u_char *cookie, const struct pcap_pkthdr *header, const u_char *packet)
{
    auto* self = (InterfaceMonitor*)cookie;

    annotator::IFMessage message;
    message.set_type(annotator::IFMessage_MessageType_UNWANTED_MESSAGE);

    // Grab the timestamps from the capture info (& make it microseconds)
    uint64_t microseconds = (header->ts.tv_sec * (uint64_t)1000000) + (header->ts.tv_usec );
    message.set_timestamp_packetcaptured(microseconds);

    u_char* payload;
    payload = parseEthernetIpTcpHeaders(packet, message);

    if (!payload){
        return;
    }

    if (message.type() != annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN &&
        message.type() != annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST)
    {
        // Message if HTTP or TLS
        if (isHttp(payload)) {
            message.set_type(annotator::IFMessage_MessageType_HTTP);
            uint32_t payload_size = header->len - (payload - packet);
            std::string payloadStr((char *) payload, payload_size);
            parseHTTPPayload(payloadStr, message);
        } else {
            message.set_type(annotator::IFMessage_MessageType_TLS_NEW_CONNECTION);
            parseTlsPayload(payload, message);
        }
    }

    if (message.type() != annotator::IFMessage_MessageType_UNWANTED_MESSAGE) {

        // Send it to Joiner
        try {
            protoSend(message, &(self->sockFd));
            LoggerCsv::log(message, "/Users/jan.kala/InterfaceMonitor.csv");
        } catch (SendMessageFailed& e){
            LoggerCsv::log(message, "/Users/jan.kala/InterfaceMonitor.csv", "FAILED_TO_SEND!");
        }

    }

}

u_char *
InterfaceMonitor::parseEthernetIpTcpHeaders(const u_char *packet, annotator::IFMessage &message) {
    uint16_t size_ethernet_header = 14;
    uint16_t size_ip_header;
    uint16_t size_tcp_header;

    // Skip the Ethernet header and read IP header
    auto ip = (struct ip*)(packet + size_ethernet_header);
    if (ip->ip_v == 4) {
        size_ip_header = parseIpV4Header(ip, message);
    } else if (ip->ip_v == 6){
        size_ip_header = parseIpV6Header((ip6_hdr *) ip, message);
    } else {
        throw std::runtime_error("Unsupported IP version!");
    }

    // Move to the TCP headers
    auto tcp = (struct tcphdr*) (packet + size_ethernet_header + size_ip_header);
    size_tcp_header = parseTcpHeader(tcp, message);

    // return pointer to the payload
    return (u_char *)(packet + size_ethernet_header + size_ip_header + size_tcp_header);
}

uint16_t
InterfaceMonitor::parseIpV4Header(const ip *ipPayload, annotator::IFMessage &message) {
    uint16_t hdrSize = ipPayload->ip_hl * 4;
    // Check the size of IP header
    if (hdrSize < 20){
        throw std::runtime_error("Invalid IP header size");
    }
    message.set_ipversion(4);

    // Grab the IPs from the header
    message.set_srcv4(ipPayload->ip_src.s_addr);
    message.set_dstv4(ipPayload->ip_dst.s_addr);

    return hdrSize;
}

uint16_t
InterfaceMonitor::parseIpV6Header(const ip6_hdr *ipPayload, annotator::IFMessage &message) {
    // IPv6 header length is always 40 bytes and it's not included in the header anyway
    // so no check for header size
    message.set_ipversion(6);

    auto srcip = (const char *)&ipPayload->ip6_src;
    auto dstip = (const char *)&ipPayload->ip6_dst;

    // Grab the IPs from the header
    message.set_srcv6(srcip, 16);
    message.set_dstv6(dstip, 16);

    return 40;
}

uint16_t
InterfaceMonitor::parseTcpHeader(const struct tcphdr *tcpPayload, annotator::IFMessage &message){
    uint16_t size_tcp_header = tcpPayload->th_off * 4;
    if (size_tcp_header < 20) {
        throw std::runtime_error("Invalid TCP header size");
    }

    // grab the ports
    message.set_srcport(ntohs(tcpPayload->th_sport));
    message.set_dstport( ntohs(tcpPayload->th_dport));

    // Set the correct TCP type if any
    if (tcpPayload->th_flags & TH_FIN) {
        message.set_type(annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN);
    }
    // RST has higher priority, so it can rewrite FIN message type
    if (tcpPayload->th_flags & TH_RST){
        message.set_type(annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST);
    }

    return size_tcp_header;
}

bool
InterfaceMonitor::isHttp(const u_char *payload) {
    // Look for GET method
    return (payload[0] == 'G' && payload[1] == 'E' && payload[2] =='T');
}

void
InterfaceMonitor::parseHTTPPayload(std::string &payload, annotator::IFMessage &message) {
    // Clean the payload of all the \r
    payload.erase(std::remove(payload.begin(), payload.end(), '\r'), payload.end());

    std::stringstream payloadStream(payload);
    std::string line;
    std::vector<std::string> lines;

    // split into lines
    while (std::getline(payloadStream, line, '\n')){
        lines.push_back(line);
    }

    // Take the resource path from first line
    std::string resourcePath = "";
    auto startOfPath = lines[0].find(' ');
    auto endOfPath = lines[0].find(" HTTP") - 1;
    resourcePath = lines[0].substr(startOfPath+1, endOfPath-startOfPath);

    // Take the host
    std::string host = "";
    for (auto &line : lines){
        if (line.find("Host: ") != std::string::npos){
            host = line.substr(6, std::string::npos);
        }
    }

    message.set_servername(host + resourcePath);
}

bool
InterfaceMonitor::parseTlsPayload(u_char *payload, annotator::IFMessage &message) {
    int sniLen;
    std::string serverName;
    char* sniStart = getTlsSni((u_char*)payload, &sniLen);
    if (sniStart){
        serverName = std::string(sniStart, sniLen);
    } else {
        serverName = "[no server name]"; // logging purpose
        message.set_type(annotator::IFMessage_MessageType_UNWANTED_MESSAGE);
    }
    message.set_servername(serverName);
    return true;
}

char *
InterfaceMonitor::getTlsSni(u_char *bytes, int* len)
{
    u_char *curr;
    u_char sidlen = bytes[43];
    curr = bytes + 1 + 43 + sidlen;
    u_short cslen = ntohs(*(u_short*)curr);
    curr += 2 + cslen;
    u_char cmplen = *curr;
    curr += 1 + cmplen;
    u_char *maxchar = curr + 2 + ntohs(*(u_short*)curr);
    curr += 2;
    u_short ext_type = 1;
    u_short ext_len;
    while(curr < maxchar && ext_type != 0)
    {
        ext_type = ntohs(*(u_short*)curr);
        curr += 2;
        ext_len = ntohs(*(u_short*)curr);
        curr += 2;
        if(ext_type == 0)
        {
            curr += 3;
            u_short namelen = ntohs(*(u_short*)curr);
            curr += 2;
            *len = namelen;
            return (char*)curr;
        }
        else curr += ext_len;
    }
    if (curr != maxchar) return nullptr;
    return nullptr; //SNI was not present
}
