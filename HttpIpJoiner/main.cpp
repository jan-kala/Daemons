#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../Utils/LoggerCsv.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"

#define SOCKET_PATH_IFMONITOR  "/tmp/IFMonitor"
int main() {
    int sock = 0;
    struct sockaddr_un local, remote;

    if ( (sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1 ){
        std::cerr << "Joiner: Failed to create socket" << std::endl;
        return 1;
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, SOCKET_PATH_IFMONITOR);
    int data_len = sizeof(local);

    unlink(SOCKET_PATH_IFMONITOR);

    if (bind(sock, (struct sockaddr*)&local, data_len) == -1){
        std::cerr << "Joiner: Failed to bind to the socket." << std::endl;
        close(sock);
        return 1;
    }

    if (listen(sock, 100) != 0) {
        std::cerr << "Joiner: Failed to listen()" << std::endl;
        close(sock);
        return 1;
    }

    int accept_socket = 0;
    u_int sock_len = 0;
    accept_socket = accept(sock, (struct sockaddr*)&remote, &sock_len);
    if ( accept_socket == -1){
        std::cerr<< "Joiner: Failed to accept()" << std::endl;
        return 1;
    }
    int totalLen =0 , dataLen=0;
    char lenghtBuffer[4];
    char recvBuffer[1024];

    while (true) {
        if ((dataLen = recv(accept_socket, lenghtBuffer, 4, MSG_PEEK))==-1) {
            std::cerr<< "Failed to read length of the message";
            return 34567;
        };
        // READ header
        google::protobuf::uint32 size;
        google::protobuf::io::ArrayInputStream ais(lenghtBuffer, 4);
        google::protobuf::io::CodedInputStream codedInputStream(&ais);
        codedInputStream.ReadVarint32(&size); // now we have the size
        // READ rest of the body
        char payload[size+4];
        int bytecount;
        if ((bytecount = recv(accept_socket, payload, size+4, MSG_WAITSTREAM)) == -1){
            std::cerr<<"Failed to recv()"<<std::endl;
            return 23;
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

#pragma clang diagnostic pop