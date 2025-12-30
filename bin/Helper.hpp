#pragma once
#include "Nakamoto.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <openssl/sha.h>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <mutex>

class Helper {
public:
    // SHA256-Hash
    static std::string sha256(const std::string& input);

    // Datei erzeugen
    static void createNextFile(Nakamoto& node, const std::string& chain, const std::string& prevfilehash, int start, int fileindex);

    // Hexadezimal-Utilities
    static size_t hexToNumber(const std::string& hex);
    static std::string hexToText(const std::string& hex, bool space = false, bool full = false);
    static std::string hexToIP(const std::string& hex);
    static std::vector<std::string> splitBySpace(const std::string& input);
    static int checkStructure(const std::string& chain, int& fileindex, Nakamoto& node);
    static void replaceAll(std::string& str, const std::string& from, const std::string& to);
    static std::string Origin(const std::string& chain, Nakamoto& node, const nlohmann::json& tx);
};
