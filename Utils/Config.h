//
// Created by Jan Kala on 18.05.2022.
//

#ifndef HTTPDATARESENDER_CONFIG_H
#define HTTPDATARESENDER_CONFIG_H

#include "ConfigDefinitions.h"
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>

using namespace nlohmann;

class Config {
public:
    explicit Config(char* binaryPath, const char* moduleName);

    json& operator[](const char *);
    json common();
    std::filesystem::path binaryPath;
    std::filesystem::path projectRootPath;
    const char* moduleName;

private:
    json loadedConfig;

};


#endif //HTTPDATARESENDER_CONFIG_H
