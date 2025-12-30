#include "Helper.hpp"

// SHA256-Funktion
std::string Helper::sha256(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// Nächstes File erstellen
void Helper::createNextFile(Nakamoto& node, const std::string& chain, const std::string& prevfilehash, int start, int fileindex) {
    std::string blockhash = nlohmann::json::parse(node.sendRpc(R"({"jsonrpc":"1.0","id":"Kotia","method":"getblockhash","params":[)" + std::to_string(start) + R"(]})"))["result"];

    nlohmann::json container = nlohmann::json::array();
    nlohmann::json entry = {
        {"height", start},
        {"tx", nlohmann::json::array()}
    };
    container.push_back(entry);

    nlohmann::json genesis = {
        {"ahead", {
            {"height", start}, 
            {"tx", 0}, 
            {"filehash", ""}, 
            {"prevfilehash", prevfilehash}
        }},
        {"data", container}
    };

    nlohmann::json block = nlohmann::json::parse(node.sendRpc(R"({"jsonrpc":"1.0","id":"Kotia","method":"getblock","params":[")" + blockhash + R"("]})"))["result"];

    for (const auto& txid : block["tx"]) {
        nlohmann::json tx = nlohmann::json::parse(node.sendRpc(R"({"jsonrpc":"1.0","id":"Kotia","method":"getrawtransaction","params":[")" + txid.get<std::string>() + R"(", 1]})"))["result"];
        if (tx["vout"][0]["scriptPubKey"]["type"] == "nulldata") 
            entry["tx"].push_back(txid);
    }

    // SHA256 berechnen
    genesis["ahead"]["filehash"] = sha256(prevfilehash + genesis["data"].dump());

    // File schreiben
    std::ofstream file("MDB/" + chain + "/" + std::to_string(fileindex) + ".json");
    file << genesis.dump(4);
    file.close();
}

// Hex in Zahl
size_t Helper::hexToNumber(const std::string& hex) {
    return static_cast<size_t>(std::stoull(hex, nullptr, 16));
}

// Hex in Text
std::string Helper::hexToText(const std::string& hex, bool space, bool full) {
    std::string result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
        size_t val = hexToNumber(byteString);

        if ((val >= 65 && val <= 90) || (val >= 97 && val <= 122) || (space && val == 32) || full) 
            result.push_back(byte);
    }
    return result;
}

// Hex in IPv4
std::string Helper::hexToIP(const std::string& hex) {
    size_t value = hexToNumber(hex);
    unsigned char b1 = (value >> 24) & 0xFF;
    unsigned char b2 = (value >> 16) & 0xFF;
    unsigned char b3 = (value >> 8) & 0xFF;
    unsigned char b4 = value & 0xFF;

    return std::to_string(b1) + "." + std::to_string(b2) + "." +
           std::to_string(b3) + "." + std::to_string(b4);
}

std::vector<std::string> Helper::splitBySpace(const std::string& input) {
    std::vector<std::string> result;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        result.push_back(token);
    }

    return result;
}

int Helper::checkStructure(const std::string& chain, int& fileindex, Nakamoto& node) {
    //std::string prevhash = "0000000000000000000000000000000000000000000000000000000000000000";
    nlohmann::json container = nlohmann::json::array();

    if (!std::filesystem::exists("MDB")) std::filesystem::create_directory("MDB");
    if (!std::filesystem::exists("MDB/"+chain)) std::filesystem::create_directory("MDB/"+chain);

    if (!std::filesystem::exists("INDEX")) std::filesystem::create_directory("INDEX");
    if (!std::filesystem::exists("INDEX/"+chain)) std::filesystem::create_directory("INDEX/"+chain);

    if (!std::filesystem::exists("LIBARY")) std::filesystem::create_directory("LIBARY");
    if (!std::filesystem::exists("LIBARY/"+chain)) std::filesystem::create_directory("LIBARY/"+chain);

    if (!std::filesystem::exists("INDEX/" + chain + "/base.json")) {
        std::ofstream file("INDEX/" + chain + "/base.json");
        
        nlohmann::json genesis = {
            {"ahead", {
                {"height", 0}, 
                {"tx", 0}, 
                {"filehash", ""}
            }},
            {"data", container}
        };

        file << genesis.dump(4);
        file.close();
    }

    if (!std::filesystem::exists("LIBARY/"+ chain +"/lib.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/lib.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/lib.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (!std::filesystem::exists("LIBARY/"+ chain +"/tld.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/tld.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/tld.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    /*if (!std::filesystem::exists("LIBARY/"+ chain +"/ip.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/ip.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/ip.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }*/

    if (!std::filesystem::exists("LIBARY/"+ chain +"/ins.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/ins.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/ins.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (!std::filesystem::exists("LIBARY/"+ chain +"/fold.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/fold.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/fold.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (!std::filesystem::exists("LIBARY/"+ chain +"/prop.json")) {
        std::cout << "[Helper] create LIBARY/"+ chain +"/prop.json" << std::endl;
        std::ofstream f("LIBARY/"+ chain +"/prop.json");
        f << container.dump(4);
        f.close();

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // get fileindex
    try {
        for (const auto& entry : std::filesystem::directory_iterator("MDB/"+chain)) {
            if (std::filesystem::is_regular_file(entry.status())) fileindex++;
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[Helper] Fehler: " << e.what() << std::endl;
    }

    // create Genesis Block Dummy
    if (fileindex == -1) {
        std::cout << "[Helper] create Genesis Dummy" << std::endl;

        std::string result = node.sendRpc(R"({"jsonrpc":"1.0","id":"Kotia","method":"getblockhash","params":[0]})");

        nlohmann::json block = {
            {"height", 0},
            {"tx", nlohmann::json::array()}
        };

        container.push_back(block);

        nlohmann::json genesis = {
            {"ahead", {
                {"height", 1}, 
                {"tx", 0}, 
                {"filehash", ""}, 
                {"prevfilehash", "0000000000000000000000000000000000000000000000000000000000000000"}
            }},
            {"data", container}
        };

        genesis["ahead"]["filehash"] = Helper::sha256(genesis["ahead"]["prevfilehash"].dump() + Helper::sha256(container.dump()));

        std::ofstream file("MDB/"+ chain +"/0.json");
        file << genesis.dump(4);
        file.close();

        return 0;
    }
    
    if (fileindex >= 0) {
        for (size_t i = 0; i <= (size_t)fileindex; i++) {
            std::string path = "MDB/" + chain + "/" + std::to_string(i) + ".json";
            std::stringstream buffer;

            std::ifstream file(path);
            if (!file.is_open()) {
                std::cerr << "[Helper] MDB File" << std::to_string(fileindex) << ".json konnte nicht geöffnet werden!" << std::endl;
                return -2;
            }
            buffer << file.rdbuf();
            nlohmann::json fileContent = nlohmann::json::parse(buffer.str());

            std::string h = Helper::sha256(fileContent["ahead"]["prevfilehash"].dump() + Helper::sha256(fileContent["data"].dump()));
        }

        return fileindex;
    }

    return -2;
}

void Helper::replaceAll(std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return; // Vermeide Endlosschleife
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Nicht wieder dieselbe Stelle prüfen
    }
}

std::string Helper::Origin(const std::string& chain, Nakamoto& node, const nlohmann::json& tx) {
    std::string rpcS3 = R"({"jsonrpc":"1.0","id":")";
    std::string rpcS4 = R"(","method":"getrawtransaction","params":[")";
    std::string rpcS1 = rpcS3 + chain + rpcS4;
    std::string rpcS2 = R"(]})";

    std::string owner = tx["vin"][0]["txid"];
    std::string rpc = rpcS1 + owner + R"(", 1)" + rpcS2;
    std::string response;

    do {
        response = node.sendRpc(rpc);
        if (response.empty()) std::this_thread::sleep_for(std::chrono::milliseconds(250));
    } while (response.empty());

    nlohmann::json o = nlohmann::json::parse(response)["result"];
    size_t vout = tx["vin"][0]["vout"];
    owner = o["vout"][vout]["scriptPubKey"]["addresses"][0];
    return owner;
}
