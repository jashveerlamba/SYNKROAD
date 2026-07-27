#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <mutex>

#include "ui/controls/ToggleSwitch.h"

enum class ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected
};

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

struct LogEntry
{
    LogLevel level;
    std::wstring timestamp;
    std::wstring message;
};

class UIManager
{
public:
    UIManager() = default;
    ~UIManager();

    bool Create(HWND parent);
    void UpdateLayout();

    void SetConnectionStatus(ConnectionStatus status);

    // Status Bar API
    void SetVersion(const std::wstring& version);
    void SetConnectionStateText(const std::wstring& state);
    void SetNetworkInfo(const std::wstring& info);
    void SetFPS(int fps);
    void SetLatency(int latencyMs);

    // Logging Panel API
    void LogInfo(const std::wstring& message);
    void LogWarning(const std::wstring& message);
    void LogError(const std::wstring& message);
    void LogDebug(const std::wstring& message);
    void Log(LogLevel level, const std::wstring& message);
    void ClearLog();

    HWND GetContentPanelHandle() const { return m_contentPanel; }
    HWND GetStatusBarHandle() const { return m_statusBar; }

private:
    static ATOM RegisterContentPanelClass(HINSTANCE instance);
    static ATOM RegisterStatusBarClass(HINSTANCE instance);

    static LRESULT CALLBACK ContentPanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK StatusBarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    std::wstring GetFormattedTimestamp() const;
    void AppendLogToRichEdit(const LogEntry& entry);

private:
    HWND m_parent = nullptr;

    // Top Bar Controls
    HWND m_logoLabel = nullptr;
    HWND m_statusLabel = nullptr;
    ToggleSwitch m_connectToggle;

    // Main Content Panel & Logging Control
    HWND m_contentPanel = nullptr;
    HWND m_logRichEdit = nullptr;

    // Status Bar
    HWND m_statusBar = nullptr;

    ConnectionStatus m_currentStatus = ConnectionStatus::Disconnected;

    // Log Storage
    std::vector<LogEntry> m_logs;
    std::mutex m_logMutex;

    // Runtime Status Bar Values
    std::wstring m_strVersion = L"v1.0.0";
    std::wstring m_strConnState = L"Disconnected";
    std::wstring m_strNetwork = L"No Device";
    std::wstring m_strFPS = L"0 FPS";
    std::wstring m_strLatency = L"-- ms";

    // Layout Constants
    static constexpr int TOP_BAR_PADDING_X = 20;
    static constexpr int TOP_BAR_PADDING_Y = 15;
    static constexpr int LOGO_WIDTH = 180;
    static constexpr int LOGO_HEIGHT = 30;
    static constexpr int STATUS_WIDTH = 110;
    static constexpr int STATUS_HEIGHT = 25;
    static constexpr int TOGGLE_WIDTH = 70;
    static constexpr int TOGGLE_HEIGHT = 35;
    static constexpr int TOP_BAR_TOTAL_HEIGHT = 60;
    static constexpr int STATUS_BAR_HEIGHT = 28;
    static constexpr int CONTENT_MARGIN = 12;

    static constexpr const wchar_t* CONTENT_PANEL_CLASS = L"SYNKROAD_ContentPanel";
    static constexpr const wchar_t* STATUS_BAR_CLASS = L"SYNKROAD_StatusBar";
};