#include "Kurator.hpp"

// ---------------- Constructor ----------------
Kurator::Kurator(const std::string& chain) : chain(chain) {
    loadJSON("LIBARY/" + chain + "/lib.json", LIB);
    loadJSON("LIBARY/" + chain + "/tld.json", TLD);
    //loadJSON("LIBARY/" + chain + "/ip.json", IP);
    loadJSON("LIBARY/" + chain + "/fold.json", FOLD);
    loadJSON("LIBARY/" + chain + "/ins.json", INS);
    loadJSON("LIBARY/" + chain + "/prop.json", PROP);
}

std::string Kurator::chaincode(const std::string& code) {
    std::string r = "Unknown Chaincode";

    if (code == cc.Kotia) r = "Kotia";
    if (code == cc.Fairbrix) r = "Fairbrix";

    return r;
}

// ---------------- JSON Load / Save ----------------
bool Kurator::loadJSON(const std::string& path, nlohmann::json& target) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Kurator] ⚠️ Konnte Datei nicht öffnen: " << path << "\n";
        target = nlohmann::json::array();
        return false;
    }
    try { file >> target; } 
    catch (const std::exception& e) {
        std::cerr << "[Kurator] ❌ JSON Parse-Fehler in " << path << ": " << e.what() << "\n";
        target = nlohmann::json::array();
        return false;
    }
    return true;
}

bool Kurator::saveJSON(const std::string& path, const nlohmann::json& source) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Kurator] ⚠️ Konnte Datei nicht speichern: " << path << "\n";
        return false;
    }
    file << source.dump(4);
    return true;
}

void Kurator::saveAll() {
    saveJSON("LIBARY/" + chain + "/lib.json", LIB);
    saveJSON("LIBARY/" + chain + "/tld.json", TLD);
    //saveJSON("LIBARY/" + chain + "/ip.json", IP);
    saveJSON("LIBARY/" + chain + "/fold.json", FOLD);
    saveJSON("LIBARY/" + chain + "/ins.json", INS);
    saveJSON("LIBARY/" + chain + "/prop.json", PROP);
}

// ---------------- Action Functions ----------------
void Kurator::activation(size_t block, const nlohmann::json& tx, const std::string& chunk) {
    std::lock_guard<std::mutex> lock(mtx);

    nlohmann::json genesis = {
        {"Block", block},
        {"TXID", tx["txid"]},
        {"Event", "MoonBrix GENESIS"},
        {"Action", "Protocol activated"},
        {"Message", Helper::hexToText(chunk.substr(30), true)}
    };
    LIB.push_back(genesis);
    saveAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

// ---------------- TLD/IP Claim ----------------
void Kurator::ipClaim(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    bool taken = false;
    bool allowed = false;
    std::string TLDh = chunk.substr(0,18);
    std::string IPh = chunk.substr(18,8);

    /*for (const auto& i : IP)
        if (i["TLD"]==TLDh && i["Hex"]==IPh) taken = true;
    if(taken) return;*/

    for (auto& t : TLD) {
        if (t["Hex"]==TLDh) {
            if(t["Type"]=="whitelist") {
                for(const auto& a : t["List"]) if(a==owner) allowed=true;
            }
            if(t["Type"]=="blacklist") {
                allowed=true;
                for(const auto& a : t["List"]) if(a==owner) allowed=false;
            }
            if(t["Owner"]==owner) allowed=true;

            for (const auto& ip : t["IPs"]) {
                if (ip[0] == IPh) taken = true;
            }

            if (allowed && !taken) {
                nlohmann::json abc = {IPh, owner, ""}; 
                t["IPs"].push_back(abc);

                FOLD.push_back({
                    {"Key", TLDh + IPh}, {"Owner", owner},
                    {"root", nlohmann::json::object()}
                });

                saveAll();
            }
        }
    }
    /*if(!allowed) return;

    nlohmann::json entry = {
        {"Block", block},
        {"TXID", tx["txid"]},
        {"Event", "claim IP"},
        {"Action", "IP " + Helper::hexToIP(IPh) + " claimed"},
        {"TLD", Helper::hexToText(TLDh)}
    };
    LIB.push_back(entry);

    IP.push_back({
        {"Hex", IPh}, {"IPv4", Helper::hexToIP(IPh)}, {"Owner", owner},
        {"Block", block}, {"TXID", tx["txid"]}, {"TLD", TLDh}, {"Name", ""}
    });*/
}

void Kurator::tldClaim(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);

    std::string TLDh = chunk.substr(0,18);
    std::string TLDs = Helper::hexToText(TLDh);
    std::string IPh = chunk.substr(18,8);

    bool givenaway = false;
    
    if (TLDh == ac.firstTLD || TLDh == ac.lastTLD) return;

    for (const nlohmann::json& t : TLD) {
        if (t["Ascii"] == TLDs) givenaway = true;
    }

    if (givenaway) {
        ipClaim(block, tx, chunk, owner);
        return;
    }
    if (IPh != ac.firstIP) return;

    LIB.push_back({
        {"Block", block},
        {"TXID", tx["txid"]},
        {"Event", "claim TLD"},
        {"Action", "TLD " + TLDs + " claimed"},
        {"Message", Helper::hexToText(chunk.substr(30))}
    });

    TLD.push_back({
        {"Hex", TLDh},
        {"Ascii", TLDs},
        {"Owner", owner},
        {"Block", block},
        {"TXID", tx["txid"]},
        {"Type", "whitelist"},
        {"List", nlohmann::json::array()},
        {"IPs", nlohmann::json::array()}
    });

    saveAll();
}

// ---------------- Change Owner ----------------
void Kurator::changeOwner(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    if(tx["vout"].size()<3) return;

    std::string newowner=tx["vout"][2]["scriptPubKey"]["addresses"][0], TLDh=chunk.substr(0,18);
    bool change=false;

    for(auto& t:TLD) if(t["Hex"]==TLDh && t["Owner"]==owner) {
        t["Owner"]=newowner;
        change=true;
        break;
    }
    if(!change) return;

    LIB.push_back({
        {"Block", block}, {"TXID", tx["txid"]},
        {"Event", "change owner"},
        {"Action", "TLD " + Helper::hexToText(TLDh) + " owner changed"},
        {"TLD", Helper::hexToText(TLDh)}
    });

    saveAll();
}

// ---------------- Change List Type ----------------
void Kurator::changeListType(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);

    std::string TLDh=chunk.substr(0,18), listtype, IPh=chunk.substr(18,8);
    if(chunk.size()!=32||IPh!=ac.firstIP) return;

    listtype = (chunk.substr(30,2)=="00") ? "whitelist" : "blacklist";
    bool change=false;

    for(auto& t:TLD) if(t["Hex"]==TLDh && t["Owner"]==owner) {
        t["Type"]=listtype;
        change=true;
    }
    if(!change) return;

    LIB.push_back({
        {"Block", block}, {"TXID", tx["txid"]},
        {"Event", "change type"},
        {"Action", "Type changed to "+listtype},
        {"TLD", Helper::hexToText(TLDh)}
    });

    saveAll();
}

// ---------------- Change List Content ----------------
void Kurator::changeListContent(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string TLDh=chunk.substr(0,18), subaction=chunk.substr(30,2), address, IPh=chunk.substr(18,8);
    if(chunk.size()!=32||IPh!=ac.firstIP) return;

    bool change=false;
    if(subaction=="00" && tx["vout"].size()>2) { // add
        address = tx["vout"][2]["scriptPubKey"]["addresses"][0];
        for(auto& t:TLD) if(t["Hex"]==TLDh && t["Owner"]==owner) { t["List"].push_back(address); change=true; }
    }
    if(subaction=="01" && tx["vout"].size()>2) { // remove
        address = tx["vout"][2]["scriptPubKey"]["addresses"][0];
        for(auto& t:TLD) if(t["Hex"]==TLDh && t["Owner"]==owner) {
            auto& lst=t["List"];
            lst.erase(std::remove(lst.begin(),lst.end(),address),lst.end());
            change=true;
        }
    }
    if(subaction=="02" && tx["vout"].size()>1) { // clear
        for(auto& t:TLD) if(t["Hex"]==TLDh && t["Owner"]==owner) { t["List"]=nlohmann::json::array(); change=true; }
    }
    if(!change) return;

    LIB.push_back({
        {"Block", block}, {"TXID", tx["txid"]},
        {"Event", "change list"},
        {"Action", "List have been changed"},
        {"TLD", Helper::hexToText(TLDh)}
    });

    saveAll();
}

// ---------------- Change IP Name ----------------
void Kurator::changeIPName(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string TLDh=chunk.substr(0,18), IPh=chunk.substr(18,8), name=chunk.substr(30);
    if(IPh==ac.firstIP||chunk.size()<32||chunk.size()>114) return;

    //bool change=false;

    for (auto& t : TLD) {
        if (t["Hex"] == TLDh) {
            for (auto& ip : t["IPs"]) {
                if (ip[0] == IPh && ip[1] == owner) ip[2] = Helper::hexToText(name);
            }
        }
    }

    /*for(auto& i:IP) if(i["TLD"]==TLDh && i["Hex"]==IPh && i["Owner"]==owner) { i["Name"]= Helper::hexToText(name); change=true; }
    if(!change) return;

    LIB.push_back({
        {"Block", block}, {"TXID", tx["txid"]},
        {"Event", "change IP Name"},
        {"Action", "IP Name changed to "+ Helper::hexToText(name)},
        {"TLD", Helper::hexToText(TLDh)}
    });*/

    saveAll();
}

void Kurator::inscribe(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    //bool change = false;
    //bool storeINS = false;
    std::string TLDh = chunk.substr(0, 18);
    std::string IPh = chunk.substr(18, 8); //, subaction, address, IPh = chunk.substr(18, 8), data;
    if (chunk.size() < 42 || IPh == ac.firstIP || IPh == ac.lastIP || TLDh == ac.firstTLD || TLDh == ac.lastTLD) return;
    
    std::string subaction = chunk.substr(30, 2);
    //nlohmann::json LIBentry, INSentry;

    if (subaction == "00") {
        // Inscribe Text => max size depends on OP_RETURN max size
        std::string data = chunk.substr(32, chunk.size() -1);

        // not longer recorded to avoid overlarged Libary
        /*LIBentry = {
            {"Block", block},
            {"TXID", tx["txid"]},
            {"Event", "Inscription Lv0"},
            {"Action", "Inscribing chunk"},
            {"TLD", Helper::hexToText(TLDh)},
            {"IP", Helper::hexToIP(IPh)}
        };

        INSentry = {
            {"Hex", data},
            {"TXID", tx["txid"]}
        };

        change = true;*/
    }

    if (subaction == "01") {
        // Inscribe Meta => a list of inscribtion txids in correct order + Name, Filetype, follow pointer
        std::string chainstring = chaincode(chunk.substr(32, 2));
        std::string data = chunk.substr(34, chunk.size() -1);

        nlohmann::json d = nlohmann::json::parse(Helper::hexToText(data, true, true));

        if (!d["next"]) {
            // as soon Metafile > 10kB Meta files need to split via prev/next they are linked
            // only last gets recorded
            nlohmann::json LIBentry = {
                {"Block", block},
                {"TXID", tx["txid"]},
                {"Event", "Inscription"},
                {"Action", "Meta Inscribing"},
                {"TLD", Helper::hexToText(TLDh)},
                {"IP", Helper::hexToIP(IPh)}
            };

            nlohmann::json INSentry = {
                {"TXID", tx["txid"]},
                {"Name", d["name"]},
                {"Type", d["type"]}
            };

            LIB.push_back(LIBentry);
            INS.push_back(INSentry);
            saveAll();
        }
    }
}

void Kurator::folder(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::string TLDh = chunk.substr(0, 18);
    std::string IPh = chunk.substr(18, 8);
    if (chunk.size() < 62 || IPh == ac.firstIP || IPh == ac.lastIP || TLDh == ac.firstTLD || TLDh == ac.lastTLD) return;
    
    std::string subaction = chunk.substr(30, 2);
    std::string name, location;

    bool taken = false;
    nlohmann::json entry;
    
    // Root Folder is the IP
    //const std::string root = "000000000000000000000000000000";

    if (subaction == "00") {// create (Name, where?)
        name = Helper::hexToText(chunk.substr(32, 30));
        if (name.size() == 0) return;

        if (chunk.size() == 62) location = "root";
        else if (chunk.size() == 92) location = Helper::hexToText(chunk.substr(62, 30));
        else return;

        if (location.size() == 0) location = "root";

        for (nlohmann::json& e : FOLD) {
            if (e["Key"] == TLDh + IPh && e["Owner"] == owner) {
                for (nlohmann::json& n : e["root"]) {
                    for (nlohmann::json& na : n) {
                        if (to_string(na) == name) {
                            taken = true;
                            break;
                        }
                    }
                }

                if (!taken) {
                    e[location][name] = nlohmann::json::object();

                    entry = {
                        {"Block", block},
                        {"TXID", tx["txid"]},
                        {"Event", "create"},
                        {"Action", "Folder"},
                        {"TLD", Helper::hexToText(TLDh)},
                        {"IP", Helper::hexToIP(IPh)},
                        {"Location", location},
                        {"Name", name}
                    };
                    LIB.push_back(entry);

                    saveAll();
                    break;
                }
            }
        }
    }
    if (subaction == "01") {} // delete (only if empty - Name, where?)
    if (subaction == "02") {} // rename (old Name, where?)
    if (subaction == "03") {} // move (Name, where?)
    if (subaction == "04") {} // join (Name, to which folder?(Name))
}

void Kurator::file(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::string TLDh = chunk.substr(0, 18);
    std::string IPh = chunk.substr(18, 8);
    if (chunk.size() < 62 || IPh == ac.firstIP || IPh == ac.lastIP || TLDh == ac.firstTLD || TLDh == ac.lastTLD) return;

    std::string subaction = chunk.substr(30, 2);
    std::string name, location, content;
    bool taken = false;

    // Root Folder is the IP
    //const std::string root = "000000000000000000000000000000";

    if (subaction == "00") {// create (Name, Where, Content)
        name = Helper::hexToText(chunk.substr(32, 30));

        if (name.size() == 0) return;
        if (chunk.size() == 62) location = "root";
        
        if (chunk.size() == 92 || chunk.size() == 156) {
            location = Helper::hexToText(chunk.substr(62, 30));
            if (location.size() == 0) location = "root";

            if (chunk.size() == 156) {
                content = chunk.substr(92, 64);
                if (content.size() != 64) return;
            }
        }
        

        for (nlohmann::json& e : FOLD) {
            if (e["Key"] == TLDh + IPh && e["Owner"] == owner) {
                for (auto& [key, value] : e["root"].items()) {
                    for (auto& l : key) {
                        if (std::to_string(l) == location) {
                            taken = true;
                            break;
                        }
                    }
                }

                if (!taken) {
                    if (location == "") location = "root";

                    if (location == "root") e[location][name] = content;
                    else e["root"][location][name] = content;

                    nlohmann::json entry = {
                        {"Block", block},
                        {"TXID", tx["txid"]},
                        {"Event", "create"},
                        {"Action", "File"},
                        {"TLD", Helper::hexToText(TLDh)},
                        {"IP", Helper::hexToIP(IPh)},
                        {"Location", location},
                        {"Name", name}
                    };
                    LIB.push_back(entry);
                    saveAll();
                    break;
                }
            }
        }
    }

    if (subaction == "01") {// delete file (name, where)
        name = Helper::hexToText(chunk.substr(32, 30));

        if (name.size() == 0) return;
        if (chunk.size() == 62) location = "root";

        if (chunk.size() == 92) {
            location = Helper::hexToText(chunk.substr(62, 30));
            if (location.size() == 0) location = "root";
        }

        for (nlohmann::json& e : FOLD) {
            if (e["Key"] == TLDh + IPh && e["Owner"] == owner) {
                for (auto& [key, value] : e["root"].items()) {
                    if (key == name && value.is_string()) {

                        e["root"].erase(name);

                        nlohmann::json entry = {
                            {"Block", block},
                            {"TXID", tx["txid"]},
                            {"Event", "delete"},
                            {"Action", "File"},
                            {"TLD", Helper::hexToText(TLDh)},
                            {"IP", Helper::hexToIP(IPh)},
                            {"Location", location},
                            {"Name", name}
                        };
                        LIB.push_back(entry);
                        saveAll();
                        break;
                    }

                    if (key == location && value.is_object()) {
                        break;
                    }
                }
            }
        }
    }

    if (subaction == "02") {// link content

    }
}

void Kurator::property(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner) {
    std::string TLDh = chunk.substr(0, 18);
    std::string IPh = chunk.substr(18, 8);
    if (IPh == ac.firstIP || IPh == ac.lastIP || TLDh == ac.firstTLD || TLDh == ac.lastTLD || chunk.size() < 32) return;

    std::string subaction = chunk.substr(30, 2);
    bool taken = false;

    if (subaction == "00") {    // create Property
        if (chunk.size() < 86) return;
        std::string define = chunk.substr(32, 2);                           // Define Byte - Property Type
        std::string name = Helper::hexToText(chunk.substr(34, 42));         // Name length = 21 bytes (42 char)
        std::string ticker = Helper::hexToText(chunk.substr(76, 10));       // Ticker length = 5 bytes (10 char) ~ 12M combos ticker length 3-5
        
        for (char& c : ticker) {
            c = std::toupper(static_cast<unsigned char>(c));
        }

        if (name == "" || ticker == "") return;

        for (auto& o : PROP) {
            if ((o["Name"] == name) || (o["Ticker"] == ticker)) {
                taken = true;
                break;
            }
        }
        if (taken) return;

        bool fungible, divisible, fixed;

        if (define == "00") { fungible = true; divisible = false; fixed = true; }
        else if (define == "01") { fungible = true; divisible = false; fixed = false; }
        else if (define == "02") { fungible = true; divisible = true; fixed = true; }
        else if (define == "03") { fungible = true; divisible = true; fixed = false; }
        else if (define == "04") { fungible = false; divisible = false; fixed = false; }
        else return;

        nlohmann::json lib = {
            {"Block", block},
            {"TXID", tx["txid"]},
            {"Event", "Property"},
            {"Action", "create"}
        };
        LIB.push_back(lib);

        nlohmann::json prop = {
            {"Name", name},
            {"Ticker", ticker},
            {"GrantData", ""},
            {"Owner", owner},
            {"OwnerData", ""},
            {"Fungible", fungible},
            {"Divisible", divisible},
            {"Fixed", fixed},
            {"Supply", 0},
            {"Holder", nlohmann::json::object()},
            {"Key", TLDh + IPh}
        };

        //prop["GrantData"] = chunk.substr(86, 64);
        if (fixed) {
            if (chunk.size() < 102) return;

            uint64_t supply = Helper::hexToNumber(chunk.substr(86, 16)); // 8 byte (16 char) supply
            prop["Supply"] = supply;
            prop["Holder"][owner] = supply;
        }
        PROP.push_back(prop);
        saveAll();
    }
}

void Kurator::token(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& origin) {
    std::cout << "TOKEN" << std::endl;
    std::string TLDh = chunk.substr(0, 18);
    std::string IPh = chunk.substr(18, 8);
    if (IPh == ac.firstIP || IPh == ac.lastIP || TLDh == ac.firstTLD || TLDh == ac.lastTLD || chunk.size() < 32) return;

    std::string subaction = chunk.substr(30, 2);

    if (subaction == "00") {    // move Token
        std::cout << "MOVE" << std::endl;
        if (chunk.size() < 58) return;
        std::string ticker = Helper::hexToText(chunk.substr(32, 10));
        uint64_t amount = Helper::hexToNumber(chunk.substr(42, 16));

        for (char& c : ticker) {
            c = std::toupper(static_cast<unsigned char>(c));
        }

        if (ticker == "" || ticker.size() < 3) return;

        std::string destination = tx["vout"][2]["scriptPubKey"]["addresses"][0];

        for (auto& p : PROP) {
            if (p["Key"] == TLDh + IPh && p["Ticker"] == ticker && p["Holder"].contains(origin)) {
                uint64_t b = p["Holder"][origin].get<uint64_t>();

                if (b >= amount) {
                    // Sender reduzieren
                    p["Holder"][origin] = b - amount;
                    if (p["Holder"][origin].get<uint64_t>() == 0)
                        p["Holder"].erase(origin);

                    // Empfänger erhöhen oder neu anlegen
                    if (p["Holder"].contains(destination) && !p["Holder"][destination].is_null()) {
                        p["Holder"][destination] = p["Holder"][destination].get<uint64_t>() + amount;
                    } else {
                        p["Holder"][destination] = amount;
                    }

                    // Logging
                    nlohmann::json lib = {
                        {"Block", block},
                        {"TXID", tx["txid"]},
                        {"Event", "Token"},
                        {"Action", "move"}
                    };
                    LIB.push_back(lib);

                    break; // fertig mit diesem Token
                }
            }
        }
    }

    if (subaction == "01") { // mint token
        std::cout << "MINT" << std::endl;
        if (chunk.size() < 58) return;
        std::string ticker = Helper::hexToText(chunk.substr(32, 10));
        uint64_t amount = Helper::hexToNumber(chunk.substr(42, 16));

        for (char& c : ticker) {
            c = std::toupper(static_cast<unsigned char>(c));
        }

        if (ticker == "" || ticker.size() < 3) return;

        for (auto& p : PROP) {
            if (p["Key"] == TLDh + IPh && p["Ticker"] == ticker && p["Owner"] == origin) {
                p["Supply"] = p["Supply"].get<uint64_t>() + amount;
                
                if (p["Holder"].contains(origin) && !p["Holder"][origin].is_null()) {
                    std::cout << "Holder existiert schon" << std::endl;
                    p["Holder"][origin] = p["Holder"][origin].get<int64_t>() + amount;
                } else {
                    std::cout << "Holder existiert noch nicht" << std::endl;
                    p["Holder"][origin] = amount;
                }

                nlohmann::json lib = {
                    {"Block", block},
                    {"TXID", tx["txid"]},
                    {"Event", "Token"},
                    {"Action", "move"}
                };

                LIB.push_back(lib);
            }
        }
    }

    saveAll();
}

void Kurator::dex(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& origin) {
    //create order
    //cancel order
    //anounce buy
    //buy
}


// ---------------- TODO ----------------
// Die restlichen Funktionen: webServer
