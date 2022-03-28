#include <iostream>
#include <fstream>
//#include "HTTPMessage.pb.h"
#include <nlohmann/json.hpp>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define LOG_FILE_PATH "/tmp/WebAnnotator/browser.csv"

using namespace nlohmann;
//using namespace google::protobuf::util;

class Logger
{
public:
    Logger()
    {
        fd.open(LOG_FILE_PATH);
        fd.close();
    }

    void log(const std::string& text)
    {
        fd.open(LOG_FILE_PATH, std::ios::out | std::ios::app );
        fd << text << std::endl;
        fd.close();
    }

    void log(char* text)
    {
        log(std::string(text));
    }

    void csv_log(json msg)
    {
        fd.open(LOG_FILE_PATH, std::ios::out | std::ios::app );
        std::string ts_str = msg["timeStamp"].dump();
        ts_str.insert(ts_str.length()-3, ".");


        std::string url = msg["url"].dump();
        std::string proto;
        if (url.rfind("http:", 0) == 0){
            proto = "HTTP";
            url.erase(0,6);
        } else {
            proto = "TLS";
            url.erase(0,7);
        }

        fd  << ts_str << ", "
            << "Browser, "
            << proto << ", \""
            << url << "\", \""
            << msg.dump() << "\""<< std::endl;
        fd.close();
    }
private:
    std::ofstream fd;

};

Logger logger;

//void sendProtobufMessage()
//{
//    anotator::InterfaceMessage msg;
//    msg.set_check(42);
//    std::fstream fd_out("pbfile", std::ios::out | std::ios::trunc | std::ios::binary);
//    if (!msg.SerializeToOstream(&fd_out)) {
//        logger.log("failed to send message");
//    }
//}

json get_native_message() 
{
    char raw_length[4];
    fread(raw_length, 4, sizeof(char), stdin);
    uint32_t msg_length = *reinterpret_cast<uint32_t*>(raw_length);
    if(!msg_length) {
        exit(0);
    }
    char message[msg_length];
    fread(message, msg_length, sizeof(char), stdin);
    std::string m(message, message + sizeof(message) / sizeof(message[0]));
    return json::parse(m);
}

int main(int argc, char** argv)
{
//    GOOGLE_PROTOBUF_VERIFY_VERSION;

    while (true) {
        json message = get_native_message();
//        sendProtobufMessage();
        logger.csv_log(message["message"]);
    }
    return 0;
}
