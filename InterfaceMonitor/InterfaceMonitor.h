#include "LoggerCsv.h"
#include <pcapplusplus/RawPacket.h>
#include <pcapplusplus/PcapLiveDevice.h>
#include <pcapplusplus/IPv4Layer.h>
#include <pcapplusplus/TcpLayer.h>
#include <pcapplusplus/HttpLayer.h>
#include <pcapplusplus/SSLLayer.h>

class InterfaceMonitor
{
public:
    InterfaceMonitor(std::string ifname);
    int run();

private:
    std::string ifname;

    // Packets processing
    struct PacketInfo{
        bool isDesired = false;

        pcpp::IPv4Layer*    ipv4;
        pcpp::TcpLayer*     tcp;
        std::string         timestamp;
        pcpp::ProtocolType  type; // HTTP or TLS
        std::string         url; // Populated if HTTP/SSLHandshake packet is processed
    };

    static void onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie);
    static void parseIPHeaders(pcpp::RawPacket *packet, pcpp::Packet &parsedPacket,
                               struct PacketInfo &packetInProcess);
    static void parseHttpPacket(pcpp::Packet &parsedPacket, struct PacketInfo &packetInProcess);
    static void parseSslPacket(pcpp::Packet &parsedPacket, struct PacketInfo &packetInProcess);

    // Logging
    static LoggerCsv loggerCsv;
    static void logCsv(struct PacketInfo &processedPacket);
};
