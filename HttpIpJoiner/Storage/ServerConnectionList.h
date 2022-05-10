//
// Created by Jan Kala on 09.05.2022.
//

#ifndef HTTPIPJOINER_SERVERCONNECTIONLIST_H
#define HTTPIPJOINER_SERVERCONNECTIONLIST_H

#include "SocketConnectionList.h"
#include "../../ProtobufMessages/build/HTTPMessage.pb.h"

class ServerConnectionList {
public:
    struct ServerEntry{
        std::string serverNameIndicator;
        std::vector<SocketConnectionList::SocketEntry *> sockets;
        std::vector<annotator::HttpMessage> requests;
    };

    static void print(ServerEntry *entry){
        // We don't care about connections without any requests
        if (entry->requests.empty()){
            return;
        }

        std::cout << entry->serverNameIndicator << std::endl;
        for (auto it : entry->sockets){
            SocketConnectionList::print(it);
        }
        for (auto it : entry->requests){
            std::cout << "  r-> " << it.data() << std::endl;
        }
    }

    std::list<ServerEntry> connections;
};


#endif //HTTPIPJOINER_SERVERCONNECTIONLIST_H
