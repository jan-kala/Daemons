//
// Created by Jan Kala on 18.04.2022.
//

#include "MessageListeners/IFMonitorListener.h"
#include "MessageListeners/HttpDataReSenderListener.h"
#include "Dispatcher/Dispatcher.h"
#include "Storage/Storage.h"
#include <unistd.h>
#include <iostream>

int main() {
    Storage storage;

    std::string ifMonitorListenerDomainSocketPath = "/tmp/IFMonitor";
    IFMonitorListener ifMonitorListener(ifMonitorListenerDomainSocketPath, &storage);
    ifMonitorListener.run();

    std::string httpDataReSenderDomainSocketPath = "/tmp/HttpDataReSender";
    HttpDataReSenderListener httpDataReSenderListener(httpDataReSenderDomainSocketPath, &storage);
    httpDataReSenderListener.run();

    int dispatcherPortNumber = 50555;
    Dispatcher dispatcher(dispatcherPortNumber, &storage);
    dispatcher.run();

    while (true) {
        sleep(10);
    }
}
