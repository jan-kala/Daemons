//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"

int main(int argc, char** argv)
{
    // Load configuration
    Config config(argv[0], "HttpDataReSender");

    // Run the daemon
    HttpDataReSender daemon(config);
    return daemon.run();
}
