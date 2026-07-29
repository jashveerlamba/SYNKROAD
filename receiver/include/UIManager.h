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

// Custom Win32 Message for Thread-Safe Logging
#define WM_SYNKROAD_APPEND_LOG (WM_USER + 101)

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

enum class ConnectionStatus {
    Disconnected,
    Connecting,
    Connected
};

struct LogEntry {
    LogLevel level;
    std::wstring timestamp;
    std::wstring message;
};

// Custom Toggle Control Placeholder / Adapter
class ToggleSwitch {
public:
    bool Create(HWND parent, UINT id, int x, int y, int w, int h) {
        m_hwnd = CreateWindowExW(0, L"BUTTON", L"Connect",
            WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            x, y, w, h, parent, reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
            GetModuleHandleW(nullptr), nullptr);
        return m_hwnd != nullptr;
    }
    HWND Handle() const { return m_hwnd; }
    void SetChecked(bool checked) {
        if (m_hwnd) SendMessageW(m_hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    void SetOnToggle(std::function<void(bool)> callback) { m_onToggle = callback; }

private:
    HWND m_hwnd = nullptr;
    std::function<void(bool)> m_onToggle;
};

class UIManager {
public:
    UIManager() = default;
    ~UIManager();

    bool Create(HWND parent);
    void UpdateLayout();

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
    void ClearLog();

    void AppendLogToRichEdit(const LogEntry& entry);

private:
    static ATOM RegisterContentPanelClass(HINSTANCE instance);
    static ATOM RegisterStatusBarClass(HINSTANCE instance);
    static LRESULT CALLBACK ContentPanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK StatusBarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    std::wstring GetFormattedTimestamp() const;

    HWND m_parent = nullptr;
    HWND m_logoLabel = nullptr;
    HWND m_statusLabel = nullptr;
    HWND m_contentPanel = nullptr;
    HWND m_logRichEdit = nullptr;
    HWND m_statusBar = nullptr;

    ToggleSwitch m_connectToggle;
    HFONT m_statusBarFont = nullptr;

    ConnectionStatus m_currentStatus = ConnectionStatus::Disconnected;
    std::wstring m_strVersion = L"1.0.0";
    std::wstring m_strConnState = L"Disconnected";
    std::wstring m_strNetwork = L"Offline";
    std::wstring m_strFPS = L"0 FPS";
    std::wstring m_strLatency = L"-- ms";

    std::mutex m_logMutex;
    std::vector<LogEntry> m_logs;

    static constexpr wchar_t CONTENT_PANEL_CLASS[] = L"SYNKROAD_ContentPanel";
    static constexpr wchar_t STATUS_BAR_CLASS[] = L"SYNKROAD_StatusBar";

    static constexpr int TOP_BAR_PADDING_X = 16;
    static constexpr int TOP_BAR_PADDING_Y = 12;
    static constexpr int LOGO_WIDTH = 120;
    static constexpr int LOGO_HEIGHT = 28;
    static constexpr int STATUS_WIDTH = 100;
    static constexpr int STATUS_HEIGHT = 24;
    static constexpr int TOGGLE_WIDTH = 80;
    static constexpr int TOGGLE_HEIGHT = 24;
    static constexpr int TOP_BAR_TOTAL_HEIGHT = 52;
    static constexpr int STATUS_BAR_HEIGHT = 28;
    static constexpr int CONTENT_MARGIN = 12;
};