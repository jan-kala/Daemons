//
// Created by Jan Kala on 15.05.2022.
//

#include "Storage.h"

Storage::Storage(Config& config)
    : tlsPool(TlsConnectionPool(&serverHistory, &socketHistory, config[CONFIG_JOINER_ARCHIVE_PATH].get<std::string>()))
    , config(config)
    , actionLog(config)
{
    auto errorLogName = std::filesystem::path(std::string(config.moduleName) + "Error").replace_extension("csv");
    auto errorLogPath = actionLog.logFilePath;
    errorLogPath.replace_filename(errorLogName);

    errorLog = LoggerCsv(errorLogPath.string());
    IFMessageQ_mutex.unlock();
}

void Storage::processMessage(annotator::IFMessage &msg) {
    TlsConnectionPool::ActionResult res;
    switch (msg.type()) {
        case annotator::IFMessage_MessageType_HTTP:
        case annotator::IFMessage_MessageType_TLS_NEW_CONNECTION:
            res = tlsPool.add(msg);
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_FIN:
            res = tlsPool.setFinOrRemove(msg);
            break;
        case annotator::IFMessage_MessageType_TLS_CLOSE_CONNECTION_RST:
            res = tlsPool.remove(msg);
            break;
        default:
            res = TlsConnectionPool::NOP;
            break;
    }

    std::string action;
    switch (res) {
        case TlsConnectionPool::ADDED:
            action = "Added     ";
            tlsPool.printPool();
            break;
        case TlsConnectionPool::REMOVED:
            action = "Removed   ";
            tlsPool.printPool();
            break;
        case TlsConnectionPool::NOP:
            action = "Nop       ";
            break;
        case TlsConnectionPool::TCP_FIN_HOST_SET:
            action = "Fin Host  ";
            break;
        case TlsConnectionPool::TCP_FIN_CLIENT_SET:
            action = "Fin Client";
            break;
        default:
            throw std::runtime_error("Unexpected result of an action!");
            break;
    }
    actionLog.log(msg, action.c_str());
}

void Storage::processMessage(annotator::HttpMessage &msg) {
    TlsConnectionPool::ActionResult res;
    switch (msg.type()){
        case annotator::HttpMessage_MessageType_NEW_REQUEST:
            res = TlsConnectionPool::NOP;
            break;

        case annotator::HttpMessage_MessageType_PAIRING:
            res = tlsPool.addHttpRequestToServer(msg);
            break;

        default:
            res = TlsConnectionPool::NOP;
            break;
    }

    std::string action;
    switch (res) {
        case TlsConnectionPool::PAIRED:
            action = "Paired    ";
            tlsPool.succ++;
            break;
        case TlsConnectionPool::NOP:
            action = "!NO MATCH!";
            errorLog.log(msg, action.c_str());
            tlsPool.failed++;
            break;
        default:
            throw std::runtime_error("Unexpected result of an action!");
            break;
    }

    actionLog.log(msg, action.c_str());

//    auto rate = (float(succ) / (float(succ) + float(failed))) * 100;
//    std::cout << "rate: " << rate << "%" << std::endl;
}



