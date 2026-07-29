#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

#define WM_SYNKROAD_APPEND_LOG (WM_USER + 101)

enum class LogLevel { Debug, Info, Warning, Error };
enum class ConnectionStatus { Disconnected, Connecting, Connected };

struct LogEntry {
    LogLevel level;
    std::wstring timestamp;
    std::wstring message;
};

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool Initialize();
    bool Create(HWND parent);
    void UpdateLayout();
    void Resize(int width, int height);
    void HandleCommand(WPARAM wParam, LPARAM lParam);
    void UpdateConnectionStatus(ConnectionStatus status);
    void Shutdown();

    void SetConnectionStatus(ConnectionStatus status);
    void SetVersion(const std::wstring& version);
    void SetConnectionStateText(const std::wstring& state);
    void SetNetworkInfo(const std::wstring& info);
    void SetFPS(int fps);
    void SetLatency(int latencyMs);

    void LogInfo(const std::wstring& message);
    void LogWarning(const std::wstring& message);
    void LogError(const std::wstring& message);
    void LogDebug(const std::wstring& message);
    void Log(LogLevel level, const std::wstring& message);
    void AppendLog(const std::string& message);
    void ClearLog();

private:
    HWND m_parent = nullptr;
    HWND m_statusLabel = nullptr;
    HWND m_contentPanel = nullptr;
    HWND m_logRichEdit = nullptr;
    HWND m_statusBar = nullptr;

    ConnectionStatus m_currentStatus = ConnectionStatus::Disconnected;
    std::wstring m_strVersion = L"1.0.0";
    std::wstring m_strConnState = L"Disconnected";
    std::wstring m_strNetwork = L"Offline";
    std::wstring m_strFPS = L"0 FPS";
    std::wstring m_strLatency = L"-- ms";

    std::mutex m_logMutex;
    std::vector<LogEntry> m_logs;
};