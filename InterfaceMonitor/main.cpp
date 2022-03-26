#include "InterfaceMonitor.h"
#include <iostream>
#include <IPv4Layer.h>
#include <Packet.h>
#include <PcapLiveDeviceList.h>
#include <SystemUtils.h>
#include <SSLLayer.h>
#include <SSLHandshake.h>
#include <HttpLayer.h>
#include <TcpLayer.h>
#include <UdpLayer.h>
#include "../ProtobufMessages/Out/cpp/HTTPMessage.pb.h"
#include <fstream>

#define SEPARATOR ", "

struct PrintRowArgument {
    std::string timestamp; 
    std::string type; 
    std::string srcIP;
    std::string srcPort;
    std::string dstIP; 
    std::string dstPort; 
    std::string server_name;
};

void printDeviceInfo(pcpp::PcapLiveDevice* device)
{
    std::cout << "=============================================" << std::endl;
    std::cout
        << "Interface info:" << std::endl
        << "  Name:            " << device->getName() << std::endl
        << "  Description:     " << device->getDesc() << std::endl
        << "  MAC:             " << device->getMacAddress() << std::endl
        << "  Default Gateway: " << device->getDefaultGateway() << std::endl
        << "  MTU size:        " << device->getMtu() << std::endl;

    if (device->getDnsServers().size() > 0)
    {
        std::cout << "  DNS server:      " << device->getDnsServers().at(0) << std::endl;
    }
    std::cout << "=============================================" << std::endl;
}

void printTableHeader()
{
    std::string sep = SEPARATOR;
    std::cout << "TIMESTAMP\t" << sep 
              << "TYPE" << sep
              << "IP ADDRESS"  << sep
              << "PORT" << sep
              << "SERVER NAME" << std::endl;
}

void printTableRow(struct PrintRowArgument arg)
{
    std::string sep = SEPARATOR;
    std::cout<< arg.timestamp << sep 
             << "Interface" << sep
             << arg.type << ","
             << "\"" << arg.server_name << "\",\"" 
             << " src " << arg.srcIP << " : " << arg.srcPort << " |"
             << " dst " << arg.dstIP << " : " << arg.dstPort << "\""
             << std::endl;
}

void sendProtobufMessage()
{
    anotator::InterfaceMessage msg;
    msg.set_check(42);
    std::fstream fd_out("/usr/etc/test", std::ios::out | std::ios::trunc | std::ios::binary);
    if (!msg.SerializeToOstream(&fd_out)) {
        std::cerr << "failed to send message" << std::endl;
    }
}

static void onPacketArrives(pcpp::RawPacket* packet, pcpp::PcapLiveDevice* dev, void* cookie)
{
    pcpp::Packet parsedPacket(packet);
    pcpp::IPv4Layer* ipv4;
    pcpp::TcpLayer* tcp;
    std::string timestamp;

    if (parsedPacket.isPacketOfType(pcpp::IPv4))
    {
        ipv4 = parsedPacket.getLayerOfType<pcpp::IPv4Layer>();
        tcp = parsedPacket.getLayerOfType<pcpp::TcpLayer>();

        timespec ts = packet->getPacketTimeStamp();
        timestamp = std::to_string(ts.tv_sec) + "." + std::to_string(ts.tv_nsec);
    }

    if (parsedPacket.isPacketOfType(pcpp::HTTP))
    {
        pcpp::HttpRequestLayer* http_req = parsedPacket.getLayerOfType<pcpp::HttpRequestLayer>();
        if (http_req)
        {
            PrintRowArgument printArg = {
                timestamp,
                "HTTP", 
                ipv4->getSrcIPv4Address().toString(),
                std::to_string(tcp->getSrcPort()),
                ipv4->getDstIPv4Address().toString(),
                std::to_string(tcp->getDstPort()),
                http_req->getUrl()
            };
            printTableRow(printArg);
            sendProtobufMessage();
        }
    } 
    else if (parsedPacket.isPacketOfType(pcpp::SSL))
    {

        pcpp::SSLHandshakeLayer* ssl_handshake = parsedPacket.getLayerOfType<pcpp::SSLHandshakeLayer>();
        if (ssl_handshake)
        {
            pcpp::SSLClientHelloMessage* ssl_client_hello = ssl_handshake->getHandshakeMessageOfType<pcpp::SSLClientHelloMessage>();
            if (ssl_client_hello) 
            {
                pcpp::SSLServerNameIndicationExtension* ssl_sni = ssl_client_hello->getExtensionOfType<pcpp::SSLServerNameIndicationExtension>();
                if (ssl_sni)
                {
                    PrintRowArgument printArg = {
                        timestamp,
                        "TLS", 
                        ipv4->getSrcIPv4Address().toString(),
                        std::to_string(tcp->getSrcPort()),
                        ipv4->getDstIPv4Address().toString(),
                        std::to_string(tcp->getDstPort()),
                        ssl_sni->getHostName()
                    };
                    printTableRow(printArg);
                    sendProtobufMessage();
                }
                
            }
        }
    }
}

void handleSigInt(void* cookie)
{
    bool* stop = (bool*)cookie;
    *stop = true;
}

int main(int argc, char* argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    pcpp::PcapLiveDevice* device = pcpp::PcapLiveDeviceList::getInstance().getPcapLiveDeviceByName("en0");
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

    return 0;
}