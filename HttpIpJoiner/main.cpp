//
// Created by Jan Kala on 18.04.2022.
//

#include "IFMonitorListener.h"
#include "HttpDataReSenderListener.h"
#include "PairingCache.h"
#include <unistd.h>
#include <iostream>


int main() {
    PairingCache pairingCache;

    std::string ifMonitorListenerDomainSocketPath = "/tmp/IFMonitor";
    IFMonitorListener ifMonitorListener(ifMonitorListenerDomainSocketPath, &pairingCache);
    ifMonitorListener.run();

    std::string httpDataReSenderDomainSocketPath = "/tmp/HttpDataReSender";
    HttpDataReSenderListener httpDataReSenderListener(httpDataReSenderDomainSocketPath, &pairingCache);
    httpDataReSenderListener.run();

    while (true) {
        sleep(5);
        system("clear");
        while (pairingCache.cacheLock.test_and_set(std::memory_order_acquire));
        for (auto& item: pairingCache.cache){
            std::cout << item.first << std::endl;
        }
        pairingCache.cacheLock.clear(std::memory_order_release);
    }
}
