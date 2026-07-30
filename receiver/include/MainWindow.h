#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
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