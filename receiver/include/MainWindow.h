#pragma once

#include <windows.h>

#include "UIManager.h"

class MainWindow
{
public:
    MainWindow();
    ~MainWindow();

    bool Create(HINSTANCE instance, int cmdShow);
    int Run();

private:
    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    LRESULT HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

private:
    HWND m_window = nullptr;
    HINSTANCE m_instance = nullptr;

    UIManager m_uiManager;

    static constexpr const wchar_t* WINDOW_CLASS =
        L"SYNKROAD_MainWindow";

    static constexpr const wchar_t* WINDOW_TITLE =
        L"SYNKROAD Receiver";
};