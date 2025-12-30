#pragma once
#include "Helper.hpp"

class Kurator {
public:
    Kurator(const std::string& chain);

    // Public API
    void activation(size_t block, const nlohmann::json& tx, const std::string& chunk);
    void ipClaim(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void tldClaim(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void changeOwner(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void changeListType(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void changeListContent(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void changeIPName(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void inscribe(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void folder(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void file(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void property(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& owner);
    void token(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& origin);
    void dex(size_t block, const nlohmann::json& tx, const std::string& chunk, const std::string& origin);

    void saveAll();

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
        file = "07",
        property = "08",
        token = "09",
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

    std::string chaincode(const std::string& hex);

    struct ChainCode {
    std::string
        Kotia = "00",
        Fairbrix = "01";    
    };
    ChainCode cc;

private:
    std::string chain;

    // Cached JSONs
    nlohmann::json LIB, TLD, FOLD, INS, PROP;

    // Mutex for thread safety
    std::mutex mtx;

    // Helpers
    bool loadJSON(const std::string& path, nlohmann::json& target);
    bool saveJSON(const std::string& path, const nlohmann::json& source);
};
