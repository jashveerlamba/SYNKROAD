#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "ConfigManager.h"
#include <functional>
#include <string>
#include <windows.h>

class NetworkManager {
public:
    using LogCallback = std::function<void(const std::string&)>;

    NetworkManager() = default;
    ~NetworkManager() = default;

    void SetLogCallback(LogCallback callback);
    void SetNotificationWindow(HWND hwnd);
    bool Start(uint16_t port);
    bool Start(const ReceiverConfig& config);
    void Stop();

private:
    LogCallback m_logCallback;
    HWND m_notifyHwnd = nullptr;
    bool m_isRunning = false;
};