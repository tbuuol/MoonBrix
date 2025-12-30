#include "Sync.hpp"

Sync::Sync() : filesize(10000), master(filesize), index(filesize), isSyncing(false) {

}
Sync::~Sync() {
    //this->isSyncing = false;

    if (TheSync.joinable()) {
        std::cout << "[Sync] stopped" << std::endl;
        TheSync.join();
    }
}

/*struct ChainCode {
std::string
    Kotia = "00",
    Fairbrix = "01";    
};
ChainCode cc;

struct ActionCode {
std::string 
    activation = "000000000000000000000000000000",
    firstTLD = "000000000000000000",
    lastTLD = "ffffffffffffffffff", 
    firstIP = "00000000",
    lastIP = "ffffffff",
    claim = "00",
    owner = "01",
    listtype = "02",
    changelist = "03",
    name = "04",
    inscribe = "05",  
    folder = "06",
    file = "07",    // DONE
    property = "08",
    inscribefile = "09",
    createproperty = "0a",
    propertydata = "0b",
    supply = "0c",
    createNFT = "0d",
    nftownerdata = "0e",
    nftholderdata = "0f",
    dexlist = "10",
    dexcancel = "11",
    dexannounce = "12",
    dextake = "13";
};
ActionCode ac;

std::string chaincode(const std::string& code) {
    std::string r = "Unknown Chaincode";

    if (code == cc.Kotia) r = "Kotia";
    if (code == cc.Fairbrix) r = "Fairbrix";

    return r;
}*/

void Sync::Start(Nakamoto &node, std::string chain) {
    if (!this->isSyncing) return;
    std::cout << "\r\33[2K[Sync] started" << std::flush;

    std::stringstream buffer;
    int chainheight = 0;
    int fileindex = -1;

    fileindex = Helper::checkStructure(chain, fileindex, node);
    if (fileindex == -2) return;

    // read in last file
    std::string path = "MDB/" + chain + "/" + std::to_string(fileindex) + ".json";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Sync] MDB File" << std::to_string(fileindex) << ".json konnte nicht geöffnet werden! Start" << std::endl;
        return;
    }

    buffer << file.rdbuf();
    nlohmann::json fileContent = nlohmann::json::parse(buffer.str()), block, tx;
    int syncStart = fileContent["ahead"]["height"];
    int syncEnd = filesize * (fileindex +1);

    // get chain height
    std::string rpc = R"({"jsonrpc":"1.0","id":"Kotia","method":"getblockcount","params":[]})";

    do {
        std::string result = node.sendRpc(rpc);
        if (!result.empty()) {
            chainheight = nlohmann::json::parse(result)["result"];
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (true);

    if (syncStart == chainheight) {
        index.Start(node, fileindex, chain);
        BuildLibary(chain, node);

        if (this->isSyncing) std::cout << "\r\33[2K[Sync] complete, restart in 5sec" << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        //std::cout << "\033[F\33[2K"; // Cursor nach oben + Zeile löschen
        std::cout << "\33[2K";
        for (size_t i = 0; i < 2; i++) { // delete 2 Lines
            std::cout << "\033[F\33[2K";
        }
        
        Start(node, chain);
    } else {
        std::string rpc = R"({"jsonrpc":"1.0","id":"Kotia","method":"getblockhash","params":[)" + std::to_string(syncStart) + R"(]})";
        std::string blockhash = nlohmann::json::parse(node.sendRpc(rpc))["result"];

        master.Start(node, chain, fileindex, syncStart, syncEnd, blockhash, fileContent, chainheight);
        Start(node, chain);
    }
}

void Sync::Stop() {
    master.Stop();

    this->isSyncing = false;
    
    if (this->TheSync.joinable()) {
        this->TheSync.join();
    }
}


void Sync::BuildLibary(std::string chain, Nakamoto &node) {
    nlohmann::json container = nlohmann::json::array();

    Kurator kurator(chain);

    std::ifstream index("INDEX/"+ chain +"/base.json");
    std::ifstream lib("LIBARY/"+ chain +"/lib.json");
    std::ifstream tld("LIBARY/"+ chain +"/tld.json");
    //std::ifstream ip("LIBARY/"+ chain +"/ip.json");

    std::stringstream Ibuf, LIBbuf, TLDbuf;
    Ibuf << index.rdbuf();
    LIBbuf << lib.rdbuf();
    TLDbuf << tld.rdbuf();

    nlohmann::json Index = nlohmann::json::parse(Ibuf.str()), tx, o;
    nlohmann::json IndexContent = Index["data"];
    nlohmann::json IndexHeader = Index["ahead"];

    nlohmann::json Libary = nlohmann::json::parse(LIBbuf.str());
    nlohmann::json TLD = nlohmann::json::parse(TLDbuf.str());
    //nlohmann::json IP = nlohmann::json::parse(IPbuf.str());

    std::string rpcS3 = R"({"jsonrpc":"1.0","id":")";
    std::string rpcS4 = R"(","method":"getrawtransaction","params":[")";
    std::string rpcS1 = rpcS3 + chain + rpcS4;
    std::string rpcS2 = R"(]})";
    std::string txid, rpc, response;

    size_t start = 0;
    size_t end = IndexContent.size();

    if (Libary.size() > 0) {
        size_t lastlibe = Libary[Libary.size() -1]["Block"];
        for (size_t z = 0; z < IndexContent.size(); z++) {
            if (IndexContent[z]["height"] == lastlibe) start = z +1;
        }

        if (IndexContent[IndexContent.size() -1]["height"] == lastlibe) return;
    }

    std::vector<std::string> chunks;
    std::string TLDh, IPh, ACTh, SUBh, origin, TLDs;

    for (size_t a = start; a < end; a++) {
        int block = IndexContent[a]["height"];

        for (size_t b = 0; b < IndexContent[a]["tx"].size(); b++) {
            std::stringstream stream;
            std::ifstream lib("LIBARY/"+ chain +"/lib.json");
            stream << lib.rdbuf();
            nlohmann::json LIB = nlohmann::json::parse(stream.str());

            txid = IndexContent[a]["tx"];
            rpc = rpcS1 + txid + R"(", 1)" + rpcS2;

            do {
                response = node.sendRpc(rpc);
                if (response.empty()) std::this_thread::sleep_for(std::chrono::seconds(1));
            } while (response.empty());

            tx = nlohmann::json::parse(response)["result"];
            origin = Helper::Origin(chain, node, tx);

            chunks = Helper::splitBySpace(tx["vout"][0]["scriptPubKey"]["asm"]);
            if (chunks.size() == 2 && chunks[0] == "OP_RETURN" && chunks[1].size() >= 30) {
                ACTh = chunks[1].substr(26, 2);
                SUBh = chunks[1].substr(28, 2);

                if (LIB.size() == 0) {
                    if (chunks[1].substr(0, 30) == kurator.ac.activation) kurator.activation(block, tx, chunks[1]);
                } else {
                    if (ACTh == kurator.ac.claim) kurator.tldClaim(block ,tx, chunks[1], origin);
                    if (ACTh == kurator.ac.owner) kurator.changeOwner(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.listtype) kurator.changeListType(block ,tx, chunks[1], origin);
                    if (ACTh == kurator.ac.changelist) kurator.changeListContent(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.name) kurator.changeIPName(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.inscribe) kurator.inscribe(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.folder) kurator.folder(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.file) kurator.file(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.property) kurator.property(block, tx, chunks[1], origin);
                    if (ACTh == kurator.ac.token) kurator.token(block, tx, chunks[1], origin);
                }
            }
        }
    }
    return;
}
