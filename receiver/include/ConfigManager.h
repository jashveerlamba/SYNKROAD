#pragma once

#include <string>
#include <mutex>
#include <cstdint>

enum class ThemeMode
{
    Light,
    Dark,
    System
};

struct ReceiverConfig
{
    std::wstring receiverName = L"SYNKROAD Receiver";
    uint16_t listeningPort = 8080;
    bool autoStartReceiver = false;
    bool startMinimized = false;
    bool enableDebugLogging = true;
    ThemeMode theme = ThemeMode::System;
    std::wstring language = L"en-US";
};

class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager() = default;

    // Reset / Initialization
    void LoadDefaults();
    void ResetDefaults();

    // Receiver Name API
    std::wstring GetReceiverName() const;
    bool SetReceiverName(const std::wstring& name);

    // Listening Port API
    uint16_t GetListeningPort() const;
    bool SetListeningPort(uint16_t port);

    // Auto Start API
    bool IsAutoStartReceiverEnabled() const;
    void SetAutoStartReceiver(bool enable);

    // Start Minimized API
    bool IsStartMinimizedEnabled() const;
    void SetStartMinimized(bool enable);

    // Debug Logging API
    bool IsDebugLoggingEnabled() const;
    void SetDebugLoggingEnabled(bool enable);

    // Theme API
    ThemeMode GetTheme() const;
    void SetTheme(ThemeMode theme);

    // Language API
    std::wstring GetLanguage() const;
    void SetLanguage(const std::wstring& language);

    // Full Config Struct Snapshot (Thread-safe)
    ReceiverConfig GetConfig() const;

private:
    ReceiverConfig m_config;
    mutable std::mutex m_configMutex;

    static constexpr uint16_t MIN_PORT = 1024;
    static constexpr uint16_t MAX_PORT = 65535;
    static constexpr size_t MAX_NAME_LENGTH = 64;
};