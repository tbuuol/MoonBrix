#pragma once
#include "Sync.hpp"

class ApiServer {
    std::atomic<bool> Active;
    int Port;
    int Socket;
    Sync Indexer;
    std::unordered_map<std::string, std::unique_ptr<Nakamoto>> Nodes;

    void SetupSocket();                              // TCP Socket initialisieren
    void HandleRequest(int clientSocket);            // Antwort an Client
    void Commands(std::string& result, const nlohmann::json& body); // API-Befehle ausführen

public:
    ApiServer();
    ~ApiServer();

    void Start();
};
