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
        auto sockEntryJson = it->getEntryAsJson();
        sockEntryJson.erase("requests"); // We don't want to duplicate things
        sockConnsOut.push_back(sockEntryJson);
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

ServerEntry
ServerEntry::proto2serverEntry(annotator::IFMessage &msg){
    ServerEntry newEntry;
    newEntry.serverNameIndicator = msg.servername();
    return newEntry;
}
