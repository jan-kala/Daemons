//
// Created by Jan Kala on 27.03.2022.
//

#ifndef INTERFACEMONITOR_LOGGERCSV_H
#define INTERFACEMONITOR_LOGGERCSV_H

#include <iostream>
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"

class LoggerCsv {
public:

    static void log(annotator::IFMessage &message, const char *outputPath = nullptr, const char *note = nullptr);
    static void log(annotator::HttpMessage &message, const char *outputPath = nullptr, const char *note = nullptr);

protected:
    static std::ostream checkFileOutput(const char *filePath, std::ofstream &of);
};


#endif //INTERFACEMONITOR_LOGGERCSV_H
