//
// Created by Jan Kala on 27.03.2022.
//

#ifndef INTERFACEMONITOR_LOGGERCSV_H
#define INTERFACEMONITOR_LOGGERCSV_H

#include <iostream>
#include "../ProtobufMessages/build/IFMonitorMessage.pb.h"
#include "../ProtobufMessages/build/HTTPMessage.pb.h"
#include "Config.h"

#define LOG_FOLDER_NAME "log"

class LoggerCsv {
public:
    // default logger
    explicit LoggerCsv(Config &config);
    explicit LoggerCsv(const std::string& filePath);
    explicit LoggerCsv()= default;;

    void log(annotator::IFMessage &message, const char *note = nullptr);
    void log(annotator::HttpMessage &message, const char *note = nullptr);

    void fileLog(annotator::IFMessage &message, const char *outputPath, const char *note = nullptr);
    void fileLog(annotator::HttpMessage &message, const char *outputPath, const char *note = nullptr);

    static std::ostream checkFileOutput(const char *filePath, std::ofstream &of);

    bool useFile = false;
    std::filesystem::path logFilePath;
};


#endif //INTERFACEMONITOR_LOGGERCSV_H
