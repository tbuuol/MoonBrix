#include "Indexer.hpp"

Indexer::Indexer(const size_t& filesize) : Filesize(filesize) {}

void Indexer::Start(Nakamoto& node, int fileindex, std::string chain) {
    std::cout << std::endl << "[Index] Start";
    std::ifstream Index("INDEX/" + chain + "/base.json");
    if (!Index.is_open()) {
        std::cerr << "\r\33[2K[Index] Datei konnte nicht geöffnet werden" << std::flush;
        return;
    }
    
    std::string rpcS3 = R"({"jsonrpc":"1.0","id":")";
    std::string rpcS4 = R"(","method":"getrawtransaction","params":[")";
    std::string rpcS1 = rpcS3 + chain + rpcS4;
    std::string rpcS2 = R"(]})";
    std::string response, rpc, txid;
    std::stringstream buffer;
    buffer << Index.rdbuf();

    nlohmann::json IndexHeader = nlohmann::json::parse(buffer.str())["ahead"];
    nlohmann::json IndexContent = nlohmann::json::parse(buffer.str())["data"];

    int syncStart = IndexHeader["height"], txcount = IndexHeader["tx"];
    syncStart++;

    float syncFile = syncStart / this->Filesize;

    for (int a = syncFile; a <= fileindex; a++) {
        std::string filename = "MDB/" + chain + "/" + std::to_string(a) + ".json";
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "\r\33[2K[Index] Datei konnte nicht geöffnet werden" << std::flush;
            return;
        }

        std::cout << "\r\33[2K[Index] file: " << a << ".json" << std::flush;

        std::stringstream filebuffer;
        filebuffer << file.rdbuf();
        nlohmann::json fileContent = nlohmann::json::parse(filebuffer.str())["data"];


        for (const nlohmann::json& fc : fileContent) {
            if (fc["tx"].size() > 0) {
                for (const std::string tx : fc["tx"]) {
                    rpc = rpcS1 + tx + R"(", 1)" + rpcS2;

                    do {
                        response = node.sendRpc(rpc);
                        if (response.empty()) std::this_thread::sleep_for(std::chrono::seconds(1));
                    } while (response.empty());

                    nlohmann::json TX = nlohmann::json::parse(response)["result"];
                    std::string code = TX["vout"][0]["scriptPubKey"]["asm"];

                    if (code.size() >= 30) {
                        nlohmann::json entry = {
                            {"height", fc["height"]},
                            {"tx", tx},
                            {"code", Helper::splitBySpace(code)[1]}
                        };

                        if (IndexHeader["height"] < fc["height"]) {
                            IndexContent.push_back(entry);
                            
                            txcount += fc["tx"].size();
                            IndexHeader["tx"] = txcount;
                            IndexHeader["height"] = fc["height"];
                        }
                    }
                }
            }
        }
    }

    buffer << Index.rdbuf();
    nlohmann::json Indexfile = nlohmann::json::parse(buffer.str());

    IndexHeader["filehash"] = Helper::sha256("0000000000000000000000000000000000000000000000000000000000000000" + Helper::sha256(IndexContent.dump()));    

    Indexfile["ahead"] = IndexHeader;
    Indexfile["data"] = IndexContent;

    std::ofstream file("INDEX/" + chain + "/base.json");
    file << Indexfile.dump(4);
    file.close();

    std::cout << "\r\33[2K[Index] done" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
}