#include "InterfaceMonitor.h"
#include <pcapplusplus/PcapLiveDeviceList.h>
#include <pcapplusplus/SystemUtils.h>

#include <iostream>

InterfaceMonitor::InterfaceMonitor(std::string ifname) {
    this->ifname = ifname;
}

void handleSigInt(void* cookie){
    bool* stop = (bool*)cookie;
    *stop = true;
}

int InterfaceMonitor::run(){
    pcpp::PcapLiveDevice* device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName(ifname);
    if (device == nullptr)
    {
        std::cerr<< "Cannot find required device" << std::endl;
        return 1;
    }

    // ** Print info about capturing device **
    // printDeviceInfo(device);
    // printTableHeader();

    // open device for capture
    if (!device->open())
    {
        std::cerr<<"Could not open device for capture!" << std::endl;
        return 1;
    }

    bool stop = false;
    pcpp::ApplicationEventHandler::getInstance().onApplicationInterrupted(handleSigInt, &stop);

    device->startCapture(onPacketArrives, nullptr);

    while (!stop){
        pcpp::multiPlatformSleep(1);
    };

    device->stopCapture();
    device->close();
}

void InterfaceMonitor::onPacketArrives(pcpp::RawPacket *packet, pcpp::PcapLiveDevice *dev, void *cookie){
    pcpp::Packet parsedPacket(packet);
    struct PacketInfo packetInProcess = {};

    if (parsedPacket.isPacketOfType(pcpp::IPv4)){
        parseIPHeaders(packet, parsedPacket, packetInProcess);
    }

    if (parsedPacket.isPacketOfType(pcpp::HTTP)){
        parseHttpPacket(parsedPacket, packetInProcess);
    }
    else if (parsedPacket.isPacketOfType(pcpp::SSL)){
        parseSslPacket(parsedPacket, packetInProcess);
    }

    if (packetInProcess.isDesired){
        logCsv(packetInProcess);
    }
}

void InterfaceMonitor::parseIPHeaders(pcpp::RawPacket *packet, pcpp::Packet &parsedPacket,
                                      struct PacketInfo &packetInProcess) {
    packetInProcess.ipv4 = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
    packetInProcess.tcp = parsedPacket.getLayerOfType<pcpp::TcpLayer>();

    timespec ts = packet->getPacketTimeStamp();
    packetInProcess.timestamp = std::to_string(ts.tv_sec) + "." + std::to_string(ts.tv_nsec);
}

void InterfaceMonitor::parseHttpPacket(pcpp::Packet &parsedPacket, struct PacketInfo &packetInProcess) {
    auto http_req = parsedPacket.getLayerOfType<pcpp::HttpRequestLayer>();
    if (http_req)
    {
        packetInProcess.url = http_req->getUrl();
        packetInProcess.type = pcpp::HTTP;
        packetInProcess.isDesired = true;
    }
}

void InterfaceMonitor::parseSslPacket(pcpp::Packet &parsedPacket, struct PacketInfo &packetInProcess) {
    auto ssl_handshake = parsedPacket.getLayerOfType<pcpp::SSLHandshakeLayer>();
    if (ssl_handshake)
    {
        auto ssl_client_hello = ssl_handshake->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>();
        if (ssl_client_hello)
        {
            auto ssl_sni = ssl_client_hello->getExtensionOfType<pcpp::SSLServerNameIndicationExtension>();
            if (ssl_sni)
            {
                packetInProcess.url = ssl_sni->getHostName();
                packetInProcess.type = pcpp::SSL;
                packetInProcess.isDesired = true;
            }

        }
    }
}

void InterfaceMonitor::logCsv(struct PacketInfo &processedPacket) {
    LoggerCsv::InterfaceInfo info;
    info.srcIP = processedPacket.ipv4->getSrcIPv4Address().toString();
    info.srcPort = std::to_string(processedPacket.tcp->getSrcPort());
    info.dstIP = processedPacket.ipv4->getDstIPv4Address().toString();
    info.dstPort = std::to_string(processedPacket.tcp->getDstPort());
    info.server_name = processedPacket.url;
    info.timestamp = processedPacket.timestamp;
    info.type = processedPacket.type == pcpp::HTTP ? "HTTP" : "TLS";

    loggerCsv.log(info);
}
