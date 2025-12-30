#include "MasterDB.hpp"

MasterDB::MasterDB(const size_t& filesize) : Filesize(filesize), stop(false) {}

void MasterDB::Start(Nakamoto& node, std::string chain, int fileindex, int syncStart, int syncEnd, std::string blockhash, nlohmann::json fileContent, int chainheight) {
    std::cout << "[MasterDB] started";
    nlohmann::json block, tx;
    this->stop = false;

    // loop to end of file or chain
    for (int a = syncStart; a < syncEnd; a++) {
        if (this->stop) break;

        std::string rpc = R"({"jsonrpc":"1.0","id":"Kotia","method":"getblock","params":[")" + blockhash + R"("]})";
        block = nlohmann::json::parse(node.sendRpc(rpc))["result"];

        nlohmann::json entry = {
            {"height", block["height"]},
            {"tx", nlohmann::json::array()}
        };

        for (const std::string t : block["tx"]) {
            tx = nlohmann::json::parse(node.sendRpc(R"({"jsonrpc":"1.0","id":"Kotia","method":"getrawtransaction","params":[")" + t + R"(", 1]})"))["result"];
            if (tx["vout"][0]["scriptPubKey"]["type"] == "nulldata") {
                std::string asmField = tx["vout"][0]["scriptPubKey"]["asm"];
                const std::string prefix = "OP_RETURN ";

                if (asmField.rfind(prefix, 0) == 0) {
                    std::string payloadHex = asmField.substr(prefix.size());

                    if (payloadHex.size() >= 30) { // 30 Hexchars = 15 Bytes
                        entry["tx"].push_back(tx["txid"]);
                        int txcount = fileContent["ahead"]["tx"];
                        txcount++;
                        fileContent["ahead"]["tx"] = txcount;
                    }
                }
            }

            if (a > chainheight) chainheight = a;

            double per = (double)a / (double)chainheight *100;
            SyncCache = "[MasterDB] Build File " + std::to_string(fileindex) + ".json | Block " + std::to_string(a) + "/" + std::to_string(chainheight) + " | Total: " + std::to_string(per) + "%";
            std::cout << "\r\33[2K" << SyncCache << std::flush;
        }

        if ((entry["tx"].size() > 0 || (a +1) == syncEnd) && fileContent["data"][fileContent["data"].size() -1]["height"] != (syncEnd -1)) fileContent["data"].push_back(entry);

        fileContent["ahead"]["height"] = a;

        if (!block["nextblockhash"].is_null()) {
            blockhash = block["nextblockhash"];
        } else break;
    }

    fileContent["ahead"]["filehash"] = Helper::sha256(fileContent["ahead"]["prevfilehash"].dump() + Helper::sha256(fileContent["data"].dump()));

    std::ofstream file("MDB/" + chain + "/" + std::to_string(fileindex) + ".json");
    file << fileContent.dump(4);
    file.close();
    
    if (!block["nextblockhash"].is_null()) Helper::createNextFile(node, chain, fileContent["ahead"]["filehash"], syncEnd, fileindex +1);
    std::cout << std::endl;
}

void MasterDB::Stop() {
    this->stop = true;
}