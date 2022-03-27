//
// Created by Jan Kala on 27.03.2022.
//

#include "InterfaceMonitor.h"

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr<<"Please specify interface where to capture packets!"<<std::endl;
        return 1;
    }
    std::string ifname = argv[1];
    auto monitor = InterfaceMonitor(ifname);
    monitor.run();
}