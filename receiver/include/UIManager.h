#pragma once

#include <windows.h>

#include "ui/controls/ToggleSwitch.h"

class UIManager
{
public:
    UIManager() = default;
    ~UIManager() = default;

    bool Create(HWND parent);
    void UpdateLayout();

private:
    HWND m_parent = nullptr;

    // Top Bar Controls
    HWND m_logoLabel = nullptr;
    ToggleSwitch m_connectToggle;

    // Top Bar Layout Constants
    static constexpr int TOP_BAR_PADDING_X = 20;
    static constexpr int TOP_BAR_PADDING_Y = 15;
    static constexpr int LOGO_WIDTH = 180;
    static constexpr int LOGO_HEIGHT = 30;
    static constexpr int TOGGLE_WIDTH = 70;
    static constexpr int TOGGLE_HEIGHT = 35;
};