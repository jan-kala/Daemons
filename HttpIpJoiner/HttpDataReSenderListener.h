//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_HTTPDATARESENDERLISTENER_H
#define HTTPIPJOINER_HTTPDATARESENDERLISTENER_H

#include "../Utils/ProtobufReceiverBase.h"
#include "PairingCache.h"
#include <thread>
#include <mutex>

class HttpDataReSenderListener:ProtobufReceiverBase {
public:
    explicit HttpDataReSenderListener(std::string &domainSocketPath, PairingCache *pairingCache);
    ~HttpDataReSenderListener();
    void run();
private:
    void worker();

    std::thread             worker_thread;
    std::condition_variable worker_cv;
    std::mutex              worker_mutex;

    PairingCache* pairingCache;
};


#endif //HTTPIPJOINER_HTTPDATARESENDERLISTENER_H
