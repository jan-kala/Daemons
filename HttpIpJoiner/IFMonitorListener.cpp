//
// Created by Jan Kala on 18.04.2022.
//

#include "IFMonitorListener.h"
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../Utils/LoggerCsv.h"
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>

IFMonitorListener::IFMonitorListener(std::string& domainSocketPath)
    : ProtobufReceiverBase(domainSocketPath)
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
    std::chrono::seconds sec(1);

    int dataLen = 0;
    char lenghtBuffer[4];

    while (worker_cv.wait_for(lock, sec) == std::cv_status::timeout) {

        if ((dataLen = recv(this->acceptSockFd, lenghtBuffer, 4, MSG_PEEK))==-1) {
            std::cerr<< "Failed to read length of the message";
            return;
        };

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
        annotator::IFMessage message;
        message.ParseFromCodedStream(&codedInputStream2);
        codedInputStream2.PopLimit(msgLimit);
        // Print the message
        LoggerCsv::log(message);
    }
}
