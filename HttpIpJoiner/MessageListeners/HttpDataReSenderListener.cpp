//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSenderListener.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include "../Utils/LoggerCsv.h"
#include <iostream>
#include <sys/socket.h>

HttpDataReSenderListener::HttpDataReSenderListener(std::string &domainSocketPath, ActiveConnectionsPool *pool)
    : ProtobufReceiverBase(domainSocketPath)
    , pool(pool)
{}

HttpDataReSenderListener::~HttpDataReSenderListener() {
    worker_cv.notify_all();
    if (worker_thread.joinable()){
        worker_thread.join();
    }
}

void HttpDataReSenderListener::run() {
    worker_thread = std::thread(&HttpDataReSenderListener::worker, this);
}

void HttpDataReSenderListener::worker() {
    std::unique_lock<std::mutex> lock(worker_mutex);
    std::chrono::microseconds us(10);

    connectSocket();
    std::cout<< "Re-sender connected" << std::endl;

    annotator::HttpMessage message;

    while (worker_cv.wait_for(lock, us) == std::cv_status::timeout) {
        try {
            message = recvMessage<annotator::HttpMessage>();
        } catch (SocketDisconnected& e){
            connectSocket();
            std::cout<< "Re-sender connected" << std::endl;
            continue;
        }

        // Start processing
        while (true){
            // Check the IFMessage queue for incoming messages.
            pool->IFMessageQ_mutex.lock();
            if (!pool->IFMessageQ.empty()){
                // There is some IFMessage, check timestamp
                auto messageFromQ = pool->IFMessageQ.front();
                if (messageFromQ.timestamp_packetcaptured() <= message.timestamp_eventtriggered()) {
                    // IFMessage should be processed first
                    pool->processMessage(messageFromQ);
                    pool->IFMessageQ.pop();

                    pool->IFMessageQ_mutex.unlock();
                    continue;
                }
            }
            // IFMessage queue is empty or HTTPMessage should be processed next
            pool->IFMessageQ_mutex.unlock();
            break;
        }

        // Process HTTPMessage
        pool->processMessage(message);

    }
}
