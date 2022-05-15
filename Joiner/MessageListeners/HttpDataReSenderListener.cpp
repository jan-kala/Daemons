//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSenderListener.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include "../Utils/LoggerCsv.h"
#include <iostream>
#include <sys/socket.h>

HttpDataReSenderListener::HttpDataReSenderListener(std::string &domainSocketPath, Storage *storage)
    : ProtobufReceiverBase(domainSocketPath)
    , storage(storage)
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
            storage->IFMessageQ_mutex.lock();
            if (!storage->IFMessageQ.empty()){
                // There is some IFMessage, check timestamp
                auto messageFromQ = storage->IFMessageQ.front();
                if (messageFromQ.timestamp_packetcaptured() <= message.timestamp_eventtriggered()) {
                    // IFMessage should be processed first
                    storage->processMessage(messageFromQ);
                    storage->IFMessageQ.pop();

                    storage->IFMessageQ_mutex.unlock();
                    continue;
                }
            }
            // IFMessage queue is empty or HTTPMessage should be processed next
            storage->IFMessageQ_mutex.unlock();
            break;
        }

        // Process HTTPMessage
        storage->processMessage(message);

    }
}
