#include "ApiServer.hpp"

ApiServer::ApiServer() {
    // Config laden oder erstellen
    if (!std::filesystem::exists("MoonBrix.conf")) {
        nlohmann::json config = {
            {"Port", 10000},
            {"Nodes", {
                {"Kotia", {{"User","user"},{"Pass","pass"},{"IP","127.0.0.1"},{"Port",10001}}},
                {"Fairbrix", {{"User","user"},{"Pass","pass"},{"IP","127.0.0.1"},{"Port",10002}}}
            }}
        };
        std::ofstream("MoonBrix.conf") << config.dump(4);
    }

    std::ifstream file("MoonBrix.conf");
    if (!file.is_open()) {
        std::cerr << "![ApiServer] MoonBrix.conf not found" << std::endl;
        exit(EXIT_FAILURE);
    }

    nlohmann::json config = nlohmann::json::parse(file);
    file.close();

    Port = config["Port"];
    for (const auto& [key, value] : config["Nodes"].items()) {
        Nodes.emplace(key, std::make_unique<Nakamoto>(value["User"], value["Pass"], value["IP"], value["Port"]));
    }

    Active = false;
    std::cout << "[ApiServer] Init done, listen on Port: " << Port << std::flush;
}

ApiServer::~ApiServer() {
    Active = false;
    //if (Indexer.TheSync.joinable()) Indexer.TheSync.join();
    close(Socket);
}

static nlohmann::json Search(const nlohmann::json& array) {
    //std::cout << "[ApiServer] Searching for " << value << std::endl;
    nlohmann::json result = nlohmann::json::object();
    if (array[0] == "tld") {
        std::cout << "[ApiServer] Searching for TLD " << array[1] << std::endl;

        std::ifstream file("LIBARY/Kotia/tld.json");
        nlohmann::json TLD = nlohmann::json::parse(file);
        file.close();

        for (const nlohmann::json& item : TLD) {
            //std::cout << item << std::endl;

            if (item["Ascii"] == array[1]) {
                result = item;
            }
        }
        
    }
    return result;
}

void ApiServer::SetupSocket() {
    Socket = socket(AF_INET, SOCK_STREAM, 0);
    if (Socket < 0) { perror("socket"); exit(EXIT_FAILURE); }

    int flags = fcntl(Socket, F_GETFL, 0);
    fcntl(Socket, F_SETFL, flags | O_NONBLOCK);

    int opt = 1;
    setsockopt(Socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(Port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(Socket, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(Socket, 5) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    std::cout << " - Socket bound" << std::endl;
}

void ApiServer::HandleRequest(int clientSocket) {
    char buffer[4096];

    ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer));
    if (bytesRead <= 0) { close(clientSocket); return; }

    std::string request(buffer, bytesRead);
    size_t headerEnd = request.find("\r\n\r\n");
    std::string body = (headerEnd != std::string::npos) ? request.substr(headerEnd + 4) : "";

    // --- NEU: OPTIONS-Request abfangen ---
    if (request.rfind("OPTIONS", 0) == 0) {
        std::string preflight =
            "HTTP/1.1 200 OK\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
            "Access-Control-Allow-Headers: Content-Type\r\n"
            "Content-Length: 0\r\n\r\n";
        send(clientSocket, preflight.c_str(), preflight.size(), 0);
        close(clientSocket);
        return;
    }
    // --- Ende OPTIONS ---

    std::string result = R"({"error":null,"response":"Init"})";
    try {
        if(!body.empty()) {
            auto job = nlohmann::json::parse(body);
            if(job["id"] == "MoonBrix") Commands(result, job);
            else result = Nodes.at(job["id"])->sendRpc(body);
        }
    } catch(const std::exception& e) { result = R"({"error":"Exception","response":""})"; }

    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             << "Access-Control-Allow-Headers: Content-Type\r\n"
             << "Content-Length: " << result.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << result;

    send(clientSocket, response.str().c_str(), response.str().size(), 0);
    close(clientSocket);
}

void ApiServer::Start() {
    SetupSocket();
    Active = true;

    while(Active) {
        int client = accept(Socket, nullptr, nullptr);
        if (client >= 0) std::thread(&ApiServer::HandleRequest, this, client).detach();
        else if(errno != EAGAIN && errno != EWOULDBLOCK) perror("accept");
    }
}

void ApiServer::Commands(std::string& result, const nlohmann::json& body) {
    static const std::unordered_map<std::string,int> commands = {
        {"Help",1}, {"Shutdown",2}, {"SyncStart",3}, {"SyncStop",4}, {"Get",5}
    };

    auto it = commands.find(body["method"].get<std::string>());
    if (it == commands.end()) {
        result = R"({"error":"Unknown command","response":"No action taken"})";
        return;
    }

    switch(it->second) {
        case 1: result = R"({"error":null,"response":"The Helpline"})"; break;
        case 2:
            if (!Indexer.isSyncing) {
                Active = false;
                result = R"({"error":null,"response":"Shutdown done"})";
                std::cout << std::endl << "[ApiServer] Shutdown done" << std::endl;
            }
            else {
                result = R"({"error":"Shutdown blocked - Stop Sync first","response":"ApiServer Shutdown command received"})";
                //std::cout << std::endl << "[ApiServer] Shutdown blocked, stop Sync first";
            }
            break;
        case 3:
            if (!Indexer.isSyncing) {
                Indexer.isSyncing = true;
                Indexer.TheSync = std::thread(&Sync::Start, &Indexer, std::ref(*Nodes.at("Kotia")), "Kotia");
                result = R"({"error":null,"response":"Sync started"})";
            } else result = R"({"error":"Sync already running","response":"Sync Start command received"})";
            break;
        case 4:
            if (!Indexer.isSyncing) {
                //std::cout << std::endl << "[ApiServer] Nothing to Stop";
                result = R"({"error":"Nothing to Stop","response":"Sync Stop command received"})";
            } else {
                //std::cout << std::endl << "[ApiServer] Sync stop command received" << std::endl;
                Indexer.Stop();
                Indexer.isSyncing = false;
                result = R"({"error":null,"response":"Sync stopped"})";
            }
            break;
        case 5:
            std::cout << body << std::endl;
            result = Search(body["params"]).dump();
            break;
    }
}