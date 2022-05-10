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
    std::chrono::microseconds us(50);

    connectSocket();
    std::cout<< "connected" << std::endl;

    int dataLen = 0;
    char lenghtBuffer[4];

    while (worker_cv.wait_for(lock, us) == std::cv_status::timeout) {
        if ((dataLen = recv(this->acceptSockFd, lenghtBuffer, 4, MSG_PEEK))==-1) {
            std::cerr<< "Failed to read length of the message";
            return;
        };
        if (dataLen == 0){
            connectSocket();
            std::cout<< "connected" << std::endl;
            continue;
        }

        // READ header
        google::protobuf::uint32 size;
        google::protobuf::io::ArrayInputStream ais(lenghtBuffer, 4);
        google::protobuf::io::CodedInputStream codedInputStream(&ais);
        codedInputStream.ReadVarint32(&size); // now we have the size

        // READ rest of the body
        char payload[size+4];
        int bytecount;
        if ((bytecount = recv(this->acceptSockFd, payload, size+4, MSG_WAITSTREAM)) == -1){
            std::cerr<<"Failed to recv()"<<std::endl;
            return;
        }
        google::protobuf::io::ArrayInputStream ais2(payload, size+4);
        google::protobuf::io::CodedInputStream codedInputStream2(&ais2);
        codedInputStream2.ReadVarint32(&size);
        google::protobuf::io::CodedInputStream::Limit msgLimit = codedInputStream2.PushLimit(size);
        annotator::HttpMessage message;
        message.ParseFromCodedStream(&codedInputStream2);
        codedInputStream2.PopLimit(msgLimit);

        // wait for Interface messages
        std::this_thread::sleep_for(std::chrono::seconds(1));

        while (true){
            pool->messageQ_mutex.lock();
            if (!pool->messageQ.empty()){
                auto messageFromQ = pool->messageQ.front();

                if (messageFromQ.timestamp_s() <= message.timestamp_s()) {

//                    LoggerCsv::log(messageFromQ);
                    pool->processMessage(messageFromQ);

                    pool->messageQ.pop();

                    pool->messageQ_mutex.unlock();
                    continue;
                }
            }

            pool->messageQ_mutex.unlock();
            break;
        }
//        LoggerCsv::log(message);
        pool->processMessage(message);


    }
}
