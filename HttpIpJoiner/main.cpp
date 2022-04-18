//
// Created by Jan Kala on 18.04.2022.
//

#include "IFMonitorListener.h"
#include "HttpDataReSenderListener.h"
#include <unistd.h>

int main() {
    std::string ifMonitorListenerDomainSocketPath = "/tmp/IFMonitor";
    IFMonitorListener ifMonitorListener(ifMonitorListenerDomainSocketPath);
    ifMonitorListener.run();

    std::string httpDataReSenderDomainSocketPath = "/tmp/HttpDataReSender";
    HttpDataReSenderListener httpDataReSenderListener(httpDataReSenderDomainSocketPath);
    httpDataReSenderListener.run();

    while (true) {
        sleep(10);
    }
}
