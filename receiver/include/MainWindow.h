#pragma once

#include <windows.h>
#include "UIManager.h"
#include "ConfigManager.h"
#include "NetworkManager.h"

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE instance, int cmdShow);
    int Run();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

private:
    HINSTANCE m_instance = nullptr;
    HWND m_window = nullptr;

    UIManager m_uiManager;
    ConfigManager m_configManager;
    NetworkManager m_networkManager;

    static constexpr const wchar_t* WINDOW_CLASS = L"SYNKROAD_Receiver_Class";
    static constexpr const wchar_t* WINDOW_TITLE = L"SYNKROAD Receiver";
};