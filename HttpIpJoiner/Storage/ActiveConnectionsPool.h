//
// Created by Jan Kala on 21.04.2022.
//

#ifndef INTERFACEMONITOR_SOCKETCONNECTIONSPOOL_H
#define INTERFACEMONITOR_SOCKETCONNECTIONSPOOL_H

#include <map>
#include <queue>
#include <arpa/inet.h>
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include "ServerConnectionList.h"
#include "SocketConnectionList.h"

#define PRINT_OFFSET ""

typedef struct SocketState {
    SocketConnectionList::SocketEntry *socketEntry = nullptr;
    struct tcpState {
        bool FIN_client = false;
        bool FIN_host = false;
    } tcp;
} SocketState_t;
typedef struct SocketKey {
    uint16_t ipVersion;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } src;
    uint32_t srcPort;

    bool operator==(const SocketKey &state) const {
        if (state.ipVersion != ipVersion){
            return false;
        } else if (ipVersion == 4){
            return state.src.ipv4.s_addr == src.ipv4.s_addr &&
                   state.srcPort == srcPort;

        } else if (ipVersion == 6){
            bool ipv6_equal = std::memcmp(&(src.ipv6), &(state.src.ipv6), 16) == 0;

            return ipv6_equal &&
                   state.srcPort == srcPort;
        }
        return false;
    }

    bool operator<(const SocketKey &sock) const {
        if (ipVersion < sock.ipVersion) {
            return true;

        } else if (ipVersion > sock.ipVersion) {
            return false;

        } else if (ipVersion == 4) {
            bool dstIpEq = sock.src.ipv4.s_addr == src.ipv4.s_addr;
            return src.ipv4.s_addr < sock.src.ipv4.s_addr ||
                   (dstIpEq && srcPort < sock.srcPort);

        } else if (ipVersion == 6) {
            int ipv6CmpResult = std::memcmp(&(src.ipv6), &(sock.src.ipv6), 16);
            bool ipv6Equal = ipv6CmpResult == 0;
            bool outsideIpGreater = ipv6CmpResult < 0;

            return outsideIpGreater ||
                   (ipv6Equal && srcPort < sock.srcPort);
        }
        return false;
    }

} SocketKey_t;

typedef std::map<SocketKey_t, SocketState_t> Sockets_t;

typedef struct ServerConnection {
    ServerConnectionList::ServerEntry* serverEntry = nullptr;
    Sockets_t sockets;
} ServerConnection_t;
typedef std::string ServerNameKey_t;

typedef std::map<ServerNameKey_t, ServerConnection_t> ServerNames_t;
typedef struct ServerKey {
    uint16_t ipVersion;
    union {
        struct in_addr ipv4;
        struct in6_addr ipv6;
    } dst;
    uint32_t dstPort;

    bool operator==(const ServerKey &sock) const {

        if (sock.ipVersion != ipVersion){
            return false;
        } else if (ipVersion == 4){
            return sock.dst.ipv4.s_addr == dst.ipv4.s_addr &&
                   sock.dstPort == dstPort;

        } else if (ipVersion == 6){
            bool ipv6_equal = std::memcmp(&(dst.ipv6), &(sock.dst.ipv6), 16) == 0;

            return ipv6_equal &&
                   sock.dstPort == dstPort;
        }
        return false;
    }

    bool operator<(const ServerKey &sock) const {
        if (ipVersion < sock.ipVersion) {
            return true;

        } else if (ipVersion > sock.ipVersion) {
            return false;

        } else if (ipVersion == 4) {
            bool dstIpEq = sock.dst.ipv4.s_addr == dst.ipv4.s_addr;
            return dst.ipv4.s_addr < sock.dst.ipv4.s_addr ||
                   (dstIpEq && dstPort < sock.dstPort);

        } else if (ipVersion == 6) {
            int ipv6CmpResult = std::memcmp(&(dst.ipv6), &(sock.dst.ipv6), 16);
            bool ipv6Equal = ipv6CmpResult == 0;
            bool outsideIpGreater = ipv6CmpResult < 0;

            return outsideIpGreater ||
                   (ipv6Equal && dstPort < sock.dstPort);
        }
        return false;
    }


} ServerKey_t;

typedef std::map<ServerKey_t, ServerNames_t> Pool_t;

class ActiveConnectionsPool {
public:
    enum ActionResult {
        ADDED,
        REMOVED,
        TCP_FIN_CLIENT_SET,
        TCP_FIN_HOST_SET,
        NOP,
        PAIRED
    };

    void processMessage(annotator::IFMessage &message);
    void processMessage(annotator::HttpMessage &msg);

    // Actions for Interface Monitor messages
    ActionResult add(annotator::IFMessage &msg);
    ActionResult setFinOrRemove(annotator::IFMessage &msg);
    ActionResult remove(annotator::IFMessage &msg);
    ActionResult remove(Pool_t::iterator pool_it, ServerNames_t::iterator server_it, Sockets_t::iterator socket_it);

    // Actions for Http Re-Sender messages
    ActionResult addHttpRequestToServer(annotator::HttpMessage &msg);

    // Helper functions
    static ServerKey_t proto2serverKey(annotator::IFMessage &msg);
    static ServerKey_t proto2serverKey(annotator::HttpMessage &msg);
    static ServerKey_t proto2serverKeySwapped(annotator::IFMessage &msg);

    static SocketKey_t proto2socketKey(annotator::IFMessage &msg);
    static SocketKey_t proto2socketKeySwapped(annotator::IFMessage &msg);


    std::tuple<Pool_t::iterator,
            ServerNames_t::iterator,
            Sockets_t::iterator>
    findSocket(ServerKey_t *connId, ServerNameKey_t *hostname, SocketKey_t *connState);

    std::tuple<Pool_t::iterator,
            ServerNames_t::iterator,
            Sockets_t::iterator>
    findSocket(annotator::IFMessage &msg);

    ServerConnection_t *
    findServerConnection(annotator::HttpMessage &msg);

    void printPool();

    Pool_t activeConnectionPool = {};
    // stats
    int succ = 0;
    int failed = 0;

    // queue for storing messages
    std::queue<annotator::IFMessage> IFMessageQ;
    std::mutex IFMessageQ_mutex;

    ServerConnectionList serverHistory;
    SocketConnectionList socketHistory;

    static SocketConnectionList::SocketEntry proto2socketEntry(annotator::IFMessage &msg);
    static ServerConnectionList::ServerEntry proto2serverEntry(annotator::IFMessage &msg);

    std::vector<int> ifDelays = {1};
    float getAverageDelay();
};


#endif //INTERFACEMONITOR_SOCKETCONNECTIONSPOOL_H
