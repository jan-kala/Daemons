//
// Created by Jan Kala on 13.05.2022.
//

#ifndef HTTPIPJOINER_LISTENTRYBASE_H
#define HTTPIPJOINER_LISTENTRYBASE_H

#include "../../Utils/LoggerCsv.h"
#include <nlohmann/json.hpp>
#include <fstream>

using namespace nlohmann;

class ListEntryBase {
public:
    virtual void print(const char *outputFile){
        std::ofstream of;
        auto out = LoggerCsv::checkFileOutput(outputFile, of);

        out << getEntryAsJson().dump() << std::endl;
        of.close();
    }
    virtual json getEntryAsJson() = 0;
};
#endif //HTTPIPJOINER_LISTENTRYBASE_H
