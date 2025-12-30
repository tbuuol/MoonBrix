#pragma once
#include "Helper.hpp"

class Indexer {
    size_t Filesize;

public:
    Indexer(const size_t& filesize);
    
    void Start(Nakamoto& node, int fileindex, std::string chain);
};