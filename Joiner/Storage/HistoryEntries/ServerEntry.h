//
// Created by Jan Kala on 09.05.2022.
//

#ifndef HTTPIPJOINER_SERVERENTRY_H
#define HTTPIPJOINER_SERVERENTRY_H

#include "ListEntryBase.h"
#include "../../ProtobufMessages/build/HTTPMessage.pb.h"
#include <nlohmann/json.hpp>
#include <list>

using namespace nlohmann;


class ServerEntry : public ListEntryBase{
public:
    std::string serverNameIndicator;
    std::vector<ListEntryBase *> sockets;
    std::list<annotator::HttpMessage> requests;

    json getEntryAsJson() override;
    static ServerEntry proto2serverEntry(annotator::IFMessage &msg);
};


#endif //HTTPIPJOINER_SERVERENTRY_H
