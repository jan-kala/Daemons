//
// Created by Jan Kala on 13.05.2022.
//

#ifndef HTTPIPJOINER_DISPATCHER_H
#define HTTPIPJOINER_DISPATCHER_H

#include "../Storage/Storage.h"
#include <condition_variable>
#include <thread>

class Dispatcher {
public :
    explicit Dispatcher(int port, Storage *storage);
    ~Dispatcher();

    void run();

protected:
    class SocketDisconnected : std::exception{
        [[nodiscard]] const char* what() const noexcept override{
            return "Socket has been disconnected";
        }
    };
    class SendMessageFailed : std::exception{
        [[nodiscard]] const char* what() const noexcept override{
            return "Failed to send message back to client";
        }
    };

    void worker();

    void createSocket();
    void dispatchRequest();
    void sendResponse(int acceptSocket, json response);

    std::string
    readRequest(int acceptSocket);

    json
    processRequest(json &request);

    SocketFindRequest
    parseRequest(json &request);


    std::thread             worker_thread;
    std::condition_variable worker_cv;
    std::mutex              worker_mutex;

    Storage* storage;
    int port;
    int sockFd;
};


#endif //HTTPIPJOINER_DISPATCHER_H
