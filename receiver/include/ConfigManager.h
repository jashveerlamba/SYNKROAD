#pragma once

#include <string>
#include <cstdint>

struct ReceiverConfig {
    uint16_t port = 8080;
    std::string ipAddress = "127.0.0.1";
};

class ConfigManager {
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    static bool LoadConfig(const std::string& filepath, ReceiverConfig& config);
    static bool SaveConfig(const std::string& filepath, const ReceiverConfig& config);
};