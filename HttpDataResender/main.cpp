//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"

int main(int argc, char** argv)
{
    // TODO startup sequence so the path is not hardcoded
    std::string domainSocketPath = "/tmp/HttpDataReSender";
    HttpDataReSender daemon(domainSocketPath);
    return daemon.run();
}
