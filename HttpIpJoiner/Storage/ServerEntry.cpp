//
// Created by Jan Kala on 09.05.2022.
//

#include "ServerEntry.h"


json
ServerEntry::getEntryAsJson() {
    json out;

    out["serverNameIndication"] = serverNameIndicator;
    json sockConnsOut;
    for (auto it : sockets){
        sockConnsOut.push_back(it->getEntryAsJson());
    }
    json requestsOut;
    for (auto it : requests){
        json data = json::parse(it.data());
        requestsOut.push_back(data);
    }
    out["socketConnections"] = sockConnsOut;
    out["httpRequests"] = requestsOut;
    return out;
}

void ServerEntry::print(){
    // We don't care about connections without any requests
    if (requests.empty()){
        return;
    }

    std::cout << serverNameIndicator << std::endl;
    for (auto it : sockets){
        // hmmmm
//        SocketConnectionList::print(it);
    }
    for (auto it : requests){
        std::cout << "  r-> " << it.data() << std::endl;
    }
}
