//
// Created by Jan Kala on 18.05.2022.
//

#include "Config.h"
#include "ConfigDefinitions.h"

#include <nlohmann/json.hpp>
#include <fstream>

using namespace nlohmann;

Config::Config(char *binaryPath, const char* moduleName)
    : moduleName(moduleName)
    , binaryPath(std::filesystem::path(binaryPath))
    , projectRootPath(this->binaryPath.parent_path().parent_path())
{
    auto configFilePath = projectRootPath / CONFIG_FILE_NAME;

    std::ifstream ifs(configFilePath);
    json configFile = json::parse(ifs);

    loadedConfig = configFile;
}

json &Config::operator[](const char * key) {
    return loadedConfig[moduleName][key];
}

json Config::common() {
    return loadedConfig;
}
