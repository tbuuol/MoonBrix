#pragma once
#include "Helper.hpp"

class MasterDB {
    size_t Filesize; // in blocks

public:
    bool stop;
    std::string SyncCache;

    MasterDB(const size_t& filesize);
    void Start(Nakamoto &node, std::string chain, int fileindex, int syncStart, int syncEnd, std::string blockhash, nlohmann::json fileContent, int chainheight);
    void Stop();
};
