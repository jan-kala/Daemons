//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_IFMONITORLISTENER_H
#define HTTPIPJOINER_IFMONITORLISTENER_H

#include "../Utils/ProtobufReceiverBase.h"
#include "../Utils/Config.h"
#include "Storage/Storage.h"
#include <condition_variable>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <map>

class IFMonitorListener: ProtobufReceiverBase {
public:
    explicit IFMonitorListener(Config &config, Storage *storage);
    ~IFMonitorListener();
    void run();
protected:
    void worker();

    std::thread             worker_thread;
    std::condition_variable worker_cv;
    std::mutex              worker_mutex;

    Storage*    storage;
    Config&     config;
};


#endif //HTTPIPJOINER_IFMONITORLISTENER_H
