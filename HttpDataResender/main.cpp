//
// Created by Jan Kala on 18.04.2022.
//

#include "HttpDataReSender.h"

int main(int argc, char** argv)
{
    std::string domainSocketPath = "/tmp/HttpDataReSender";
    HttpDataReSender daemon(domainSocketPath);
    return daemon.run();
}
