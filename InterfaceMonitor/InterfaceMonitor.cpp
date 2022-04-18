//
// Created by Jan Kala on 03.04.2022.
//

#include "InterfaceMonitor.h"
#include "PacketDefinitions.h"
#include <iostream>
#include <pcap.h>
#include <sstream>
#include <vector>
#include <fstream>
#include <sys/un.h>

#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"

#define IFMONITOR_RETURN_SUCCESS 0
#define IFMONITOR_RETURN_ERROR 1

/**
 * BPF Filter for capturing.
 *
 * First is TLS filter for capturing Client Hello messages,
 * that contains SNI.
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
    "(proto 41 and ip[26] = 6 and ip[(ip[72]/16*4)+60]=22 and (ip[(ip[72]/16*4+5)+60]=1) and (ip[(ip[72]/16*4+9)+60]=3) and (ip[(ip[72]/16*4+1)+60]=3))" \
    "or"  \
    "(tcp and tcp[32:4] =  0x47455420)" \

int InterfaceMonitor::run() {
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

    connectSocket();

    pcap_loop(handle, -1, onPacketArrives, (u_char*)&sockFd);


    return IFMONITOR_RETURN_SUCCESS;
}

void InterfaceMonitor::onPacketArrives(u_char *cookie, const struct pcap_pkthdr *header, const u_char *packet)
{
    int* sockFd = (int*)cookie;

    annotator::IFMessage message;
    message.set_timestamp_s(header->ts.tv_sec);
    message.set_timestamp_ms(header->ts.tv_usec);

    u_char* payload;
    payload = parseEthernetIpTcpHeaders(packet, message);

    if (isHttp(payload)) {
        uint32_t payload_size = header->len - (payload - packet);
        std::string payloadStr((char*)payload, payload_size);
        parseHTTPPayload(payloadStr, message);
    } else {
        parseTlsPayload(payload, message);
    }
    LoggerCsv::log(message);
    protoSend(message, sockFd);

}

u_char * InterfaceMonitor::parseEthernetIpTcpHeaders(const u_char *packet, annotator::IFMessage &message) {
    const struct sniff_ethernet *ethernet; /* The ethernet header */
    const struct sniff_ip *ip; /* The IP header */
    const struct sniff_tcp *tcp; /* The TCP header */

    // Skip the Ethernet header
    ip = (struct sniff_ip*)(packet + SIZE_ETHERNET);

    // Check the size of IP header
    u_int sizeIp = IP_HL(ip) * 4;
    if (sizeIp < 20){
        throw std::runtime_error("Invalid IP header size");
    }

    // Grab the IPs from the header
    message.set_ipversion(IP_V(ip));
    if (IP_V(ip) == 4 ) {
        message.set_srcv4(ip->ip_src.s_addr);
        message.set_dstv4(ip->ip_dst.s_addr);
    } else {
        //TODO
        //IPv6
    }

    // Move to the TCP headers
    tcp = (struct sniff_tcp*) (packet + SIZE_ETHERNET + sizeIp);
    u_int sizeTcp = TH_OFF(tcp)*4;
    if (sizeTcp < 20) {
        throw std::runtime_error("Invalid TCP header size");
    }

    // grab the ports
    message.set_srcport(ntohs(tcp->th_sport));
    message.set_dstport( ntohs(tcp->th_dport));

    // return pointer to the payload
    return (u_char *)(packet + SIZE_ETHERNET + sizeIp + sizeTcp);
}

bool InterfaceMonitor::isHttp(const u_char *payload) {
    return (payload[0] == 'G' && payload[1] == 'E' && payload[2] =='T');
}

void InterfaceMonitor::parseHTTPPayload(std::string &payload, annotator::IFMessage &message) {
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

void InterfaceMonitor::parseTlsPayload(u_char *payload, annotator::IFMessage &message) {
    int sniLen;
    std::string serverName;
    char* sniStart = getTlsSni((u_char*)payload, &sniLen);
    if (sniStart){
        serverName = std::string(sniStart, sniLen);
    } else {
        serverName = "! no SNI present !";
    }
    message.set_servername(serverName);
}

char *InterfaceMonitor::getTlsSni(u_char *bytes, int* len)
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

void InterfaceMonitor::protoSend(annotator::IFMessage &message, int *socket) {

    size_t payloadSize = message.ByteSizeLong() + 4;
    char payload[payloadSize];
    memset(payload, '\0', payloadSize);
    google::protobuf::io::ArrayOutputStream aos(payload, payloadSize);
    google::protobuf::io::CodedOutputStream codedOutputStream(&aos);

    codedOutputStream.WriteVarint32(message.ByteSizeLong());
    message.SerializeToCodedStream(&codedOutputStream);

    if (send(*socket, payload, payloadSize, 0) < 0){
        std::cerr << "IFMonitor: Failed to send data to Joiner! " << std::endl;
    }
}

