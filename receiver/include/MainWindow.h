#pragma once

#include <windows.h>
#include <string>
#include <memory>

#include "UIManager.h"
#include "ConfigManager.h"
#include "NetworkManager.h"

// Custom Windows Message Identifiers
#define WM_USER_CONNECTION_CHANGED (WM_USER + 101)
#define WM_USER_LATENCY_UPDATE     (WM_USER + 102)

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    void RunMessageLoop();

    HWND GetHWND() const { return m_hwnd; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    HWND m_hwnd = NULL;
    UIManager m_uiManager;
    ConfigManager m_configManager;
    NetworkManager m_networkManager;
};