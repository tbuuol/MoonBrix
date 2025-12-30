#pragma once
#include <iostream>
#include <curl/curl.h>

class Nakamoto {
private:
    std::string rpcUser;
    std::string rpcPass;
    std::string rpcHost;
    int rpcPort;

    struct curl_slist* headers;
    CURL* client;
    //std::mutex rpcMutex;

public:
    Nakamoto(const std::string& user, const std::string& pass, const std::string& host, int port);
    ~Nakamoto();

    std::string sendRpc(const std::string& jsonPayload);
};
