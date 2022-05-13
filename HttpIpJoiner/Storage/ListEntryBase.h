//
// Created by Jan Kala on 13.05.2022.
//

#ifndef HTTPIPJOINER_LISTENTRYBASE_H
#define HTTPIPJOINER_LISTENTRYBASE_H

#include <nlohmann/json.hpp>

using namespace nlohmann;

class ListEntryBase {
public:
    virtual void print() = 0;
    virtual json getEntryAsJson() = 0;
};
#endif //HTTPIPJOINER_LISTENTRYBASE_H
