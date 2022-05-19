//
// Created by Jan Kala on 18.04.2022.
//

#include "MessageListeners/IFMonitorListener.h"
#include "MessageListeners/HttpDataReSenderListener.h"
#include "Dispatcher/Dispatcher.h"
#include "Storage/Storage.h"
#include "../Utils/Config.h"
#include <unistd.h>

int main(int argc, char** argv) {
    // Load config
    Config config(argv[0], "Joiner");

    Storage storage(config);

    IFMonitorListener ifMonitorListener(config, &storage);
    ifMonitorListener.run();

    HttpDataReSenderListener httpDataReSenderListener(config, &storage);
    httpDataReSenderListener.run();

    Dispatcher dispatcher(config, &storage);
    dispatcher.run();

    while (true) {
        sleep(10);
    }
}
