//
// Created by Jan Kala on 18.04.2022.
//

#include "IFMonitorListener.h"
#include <iostream>


IFMonitorListener::IFMonitorListener(std::string &domainSocketPath, ActiveConnectionsPool *pool)
    : ProtobufReceiverBase(domainSocketPath)
    , pool(pool)
{}

IFMonitorListener::~IFMonitorListener() {
    worker_cv.notify_all();
    if (worker_thread.joinable()){
        worker_thread.join();
    }
}

void IFMonitorListener::run() {
    worker_thread = std::thread(&IFMonitorListener::worker, this);
}

void IFMonitorListener::worker() {
    std::unique_lock<std::mutex> lock(worker_mutex);
    std::chrono::microseconds us(10);

    connectSocket();
    std::cout<< "IFMonitor connected" << std::endl;

    annotator::IFMessage message;

    while (worker_cv.wait_for(lock, us) == std::cv_status::timeout) {

        try {
            message = recvMessage<annotator::IFMessage>();
        } catch (SocketDisconnected& e){
            connectSocket();
            std::cout<< "IFMonitor connected" << std::endl;
            continue;
        }

        pool->IFMessageQ_mutex.lock();
        pool->IFMessageQ.push(message);
        pool->IFMessageQ_mutex.unlock();

    }
}
