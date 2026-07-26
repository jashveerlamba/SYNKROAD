#pragma once

#include <windows.h>

#include "ui/controls/ToggleSwitch.h"

enum class ConnectionStatus
{
    Disconnected,
    Connecting,
    Connected
};

class UIManager
{
public:
    UIManager() = default;
    ~UIManager();

    bool Create(HWND parent);
    void UpdateLayout();

    void SetConnectionStatus(ConnectionStatus status);

    HWND GetContentPanelHandle() const { return m_contentPanel; }

private:
    static ATOM RegisterContentPanelClass(HINSTANCE instance);
    static LRESULT CALLBACK ContentPanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    HWND m_parent = nullptr;

    // Top Bar Controls
    HWND m_logoLabel = nullptr;
    HWND m_statusLabel = nullptr;
    ToggleSwitch m_connectToggle;

    // Main Content Panel
    HWND m_contentPanel = nullptr;

    ConnectionStatus m_currentStatus = ConnectionStatus::Disconnected;

    // Top Bar Layout Constants
    static constexpr int TOP_BAR_PADDING_X = 20;
    static constexpr int TOP_BAR_PADDING_Y = 15;
    static constexpr int LOGO_WIDTH = 180;
    static constexpr int LOGO_HEIGHT = 30;
    static constexpr int STATUS_WIDTH = 110;
    static constexpr int STATUS_HEIGHT = 25;
    static constexpr int TOGGLE_WIDTH = 70;
    static constexpr int TOGGLE_HEIGHT = 35;
    static constexpr int TOP_BAR_TOTAL_HEIGHT = 60;

    static constexpr const wchar_t* CONTENT_PANEL_CLASS = L"SYNKROAD_ContentPanel";
};