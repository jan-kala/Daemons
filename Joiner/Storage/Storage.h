//
// Created by Jan Kala on 09.05.2022.
//

#ifndef HTTPIPJOINER_STORAGE_H
#define HTTPIPJOINER_STORAGE_H

#include "../Utils/Config.h"
#include "../Utils/LoggerCsv.h"
#include "ConnectionPools/TlsConnectionPool.h"
#include "HistoryEntries/ServerEntry.h"
#include "HistoryEntries/SocketEntry.h"

class Storage {
public :
    Storage(Config& config);

    void processMessage(annotator::IFMessage &msg);
    void processMessage(annotator::HttpMessage &msg);

    Config &config;
    TlsConnectionPool tlsPool;

    std::list<ServerEntry> serverHistory;
    std::list<SocketEntry> socketHistory;

    std::queue<annotator::IFMessage> IFMessageQ;
    std::mutex IFMessageQ_mutex;

    LoggerCsv actionLog;
    LoggerCsv errorLog;
};

#endif //HTTPIPJOINER_STORAGE_H
