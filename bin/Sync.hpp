#pragma once
#include "Helper.hpp"
#include "MasterDB.hpp"
#include "Indexer.hpp"
#include "Kurator.hpp"

class Sync {
    size_t filesize; // in blocks
    MasterDB master;
    Indexer index;

    void preIndex(Nakamoto& node, int fileindex, std::string chain);
    void BuildLibary(std::string chain, Nakamoto& node);
    
public:
    Sync();
    ~Sync();
    void Start(Nakamoto &node, std::string chain);
    void Stop();

    std::atomic<bool> isSyncing;
    std::thread TheSync;
    std::string SyncCache;
};
