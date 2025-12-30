#include "Nakamoto.hpp"

namespace {
    size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        auto* output = static_cast<std::string*>(userp);
        output->append(static_cast<char*>(contents), size * nmemb);
        return size * nmemb;
    }
}

Nakamoto::Nakamoto(const std::string& user, const std::string& pass, const std::string& host, int port) : rpcUser(user), rpcPass(pass), rpcHost(host), rpcPort(port), headers(nullptr) {
    client = curl_easy_init();
    if (!client) {
        std::cerr << "[Nakamoto] curl_easy_init() fehlgeschlagen!" << std::endl;
        return;
    }

    curl_easy_setopt(client, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(client, CURLOPT_USERPWD, (rpcUser + ":" + rpcPass).c_str());
    curl_easy_setopt(client, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(client, CURLOPT_FORBID_REUSE, 0L);
    curl_easy_setopt(client, CURLOPT_TIMEOUT, 10L);

    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(client, CURLOPT_HTTPHEADER, headers);
}

Nakamoto::~Nakamoto() {
    if (client) curl_easy_cleanup(client);
    if (headers) curl_slist_free_all(headers);
}

std::string Nakamoto::sendRpc(const std::string& jsonPayload) {
    //std::cout << "JSON-" << jsonPayload << std::endl;
    
    //std::lock_guard<std::mutex> lock(rpcMutex);
    std::string response;

    if (!client) {
        std::cerr << "[Nakamoto] CURL-Client nicht initialisiert!" << std::endl;
        return response;
    }

    std::string url = "http://" + rpcHost + ":" + std::to_string(rpcPort);
    curl_easy_setopt(client, CURLOPT_URL, url.c_str());
    curl_easy_setopt(client, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(client, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(client);
    if (res != CURLE_OK) {
        std::cerr << "[Nakamoto] CURL-Fehler: " << curl_easy_strerror(res) << std::endl;
        return response;
    }

    long http_code = 0;
    curl_easy_getinfo(client, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        std::cerr << "[Nakamoto] HTTP-Status: " << http_code << std::endl;
    }

    return response;
}
