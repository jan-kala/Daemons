//
// Created by Jan Kala on 15.05.2022.
//

#ifndef HTTPIPJOINER_HTTPCONNECTIONPOOL_H
#define HTTPIPJOINER_HTTPCONNECTIONPOOL_H

#include "../../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../HistoryEntries/SocketEntry.h"
#include "../HistoryEntries/ServerEntry.h"

class HttpConnectionPool {
    HttpConnectionPool(std::list<ServerEntry> *serverHistory,
                       std::list<SocketEntry> *socketHistory);

    void add(annotator::IFMessage &msg);

    std::map<std::string, SocketEntry*> pool = {};

    std::list<ServerEntry> *serverHistory;
    std::list<SocketEntry> *socketHistory;

};


#endif //HTTPIPJOINER_HTTPCONNECTIONPOOL_H
