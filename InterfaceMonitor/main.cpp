//
// Created by Jan Kala on 27.03.2022.
//

#include "InterfaceMonitorLibpcap.h"
#include <iostream>

int main(int argc, char** argv)
{
    auto monitor = InterfaceMonitorLibpcap();
    monitor.run();
}