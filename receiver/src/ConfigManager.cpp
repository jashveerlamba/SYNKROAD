#include "ConfigManager.h"
#include <algorithm>

ConfigManager::ConfigManager()
{
    LoadDefaults();
}

void ConfigManager::LoadDefaults()
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config = ReceiverConfig{};
}

void ConfigManager::ResetDefaults()
{
    LoadDefaults();
}

std::wstring ConfigManager::GetReceiverName() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.receiverName;
}

bool ConfigManager::SetReceiverName(const std::wstring& name)
{
    if (name.empty() || name.length() > MAX_NAME_LENGTH)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.receiverName = name;
    return true;
}

uint16_t ConfigManager::GetListeningPort() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.listeningPort;
}

bool ConfigManager::SetListeningPort(uint16_t port)
{
    if (port < MIN_PORT || port > MAX_PORT)
    {
        return false;
    }

    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.listeningPort = port;
    return true;
}

bool ConfigManager::IsAutoStartReceiverEnabled() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.autoStartReceiver;
}

void ConfigManager::SetAutoStartReceiver(bool enable)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.autoStartReceiver = enable;
}

bool ConfigManager::IsStartMinimizedEnabled() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.startMinimized;
}

void ConfigManager::SetStartMinimized(bool enable)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.startMinimized = enable;
}

bool ConfigManager::IsDebugLoggingEnabled() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.enableDebugLogging;
}

void ConfigManager::SetDebugLoggingEnabled(bool enable)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.enableDebugLogging = enable;
}

ThemeMode ConfigManager::GetTheme() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.theme;
}

void ConfigManager::SetTheme(ThemeMode theme)
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.theme = theme;
}

std::wstring ConfigManager::GetLanguage() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config.language;
}

void ConfigManager::SetLanguage(const std::wstring& language)
{
    if (language.empty()) return;

    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config.language = language;
}

ReceiverConfig ConfigManager::GetConfig() const
{
    std::lock_guard<std::mutex> lock(m_configMutex);
    return m_config;
}