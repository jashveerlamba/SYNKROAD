#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>

class MainWindow {
public:
    MainWindow() = default;
    ~MainWindow() = default;

    static MainWindow* Create();
    bool Initialize();
    int Run();
    void Shutdown();

private:
    HWND m_hwnd = nullptr;
};