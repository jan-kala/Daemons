//
// Created by Jan Kala on 27.03.2022.
//

#ifndef INTERFACEMONITOR_LOGGERCSV_H
#define INTERFACEMONITOR_LOGGERCSV_H

#include <iostream>

class LoggerCsv {
public:
    struct InterfaceInfo {
        std::string timestamp;
        std::string type;
        std::string srcIP;
        std::string srcPort;
        std::string dstIP;
        std::string dstPort;
        std::string server_name;
    };

    static void log(struct InterfaceInfo info);
};


#endif //INTERFACEMONITOR_LOGGERCSV_H
