//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_HTTPDATARESENDERLISTENER_H
#define HTTPIPJOINER_HTTPDATARESENDERLISTENER_H

#include "../Utils/ProtobufReceiverBase.h"
#include "Storage/Storage.h"
#include "../../Utils/Config.h"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <vector>

class HttpDataReSenderListener:ProtobufReceiverBase {
public:
    explicit HttpDataReSenderListener(Config &config, Storage *storage);
    ~HttpDataReSenderListener();
    void run();
private:
    void worker();

    Config&                 config;
    std::thread             worker_thread;
    std::condition_variable worker_cv;
    std::mutex              worker_mutex;

    Storage *storage;
};


#endif //HTTPIPJOINER_HTTPDATARESENDERLISTENER_H
