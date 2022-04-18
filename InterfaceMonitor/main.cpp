//
// Created by Jan Kala on 27.03.2022.
//

#include "InterfaceMonitor.h"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Only one argument specifying path to Protobuf communication file needs o be specified" << std::endl;
        return 1;
    }
    std::string protoFile = argv[1];
    auto monitor = InterfaceMonitor(protoFile);
    monitor.run();
}