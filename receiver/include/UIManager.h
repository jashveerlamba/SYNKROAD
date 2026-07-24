#pragma once

#include <windows.h>

class UIManager
{
public:
    UIManager() = default;
    ~UIManager() = default;

    bool Create(HWND parent);

private:
    HWND m_parent = nullptr;

    // Top Bar
    HWND m_logoLabel = nullptr;
    HWND m_connectButton = nullptr;
};