//
// Created by Jan Kala on 18.04.2022.
//

#ifndef HTTPIPJOINER_IFMONITORLISTENER_H
#define HTTPIPJOINER_IFMONITORLISTENER_H

#include "../Utils/ProtobufReceiverBase.h"
#include <thread>
#include <mutex>

class IFMonitorListener: ProtobufReceiverBase {
public:
    explicit IFMonitorListener(std::string& domainSocketPath);
    ~IFMonitorListener();
    void run();
private:
    void worker();

    std::thread             worker_thread;
    std::condition_variable worker_cv;
    std::mutex              worker_mutex;
};


#endif //HTTPIPJOINER_IFMONITORLISTENER_H
