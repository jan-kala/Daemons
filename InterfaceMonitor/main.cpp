//
// Created by Jan Kala on 27.03.2022.
//

#include "InterfaceMonitor.h"
#include <iostream>
#include "../../Utils/Config.h"

int main(int argc, char** argv)
{
    Config config(argv[0], "InterfaceMonitor");

    auto monitor = InterfaceMonitor(config);
    monitor.run();
}