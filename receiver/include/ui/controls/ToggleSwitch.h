#pragma once

#include <windows.h>
#include <functional>

#include "ui/controls/ControlBase.h"

class ToggleSwitch : public ControlBase
{
public:
    ToggleSwitch();
    ~ToggleSwitch() override;

    bool Create(
        HWND parent,
        int id,
        int x,
        int y,
        int width,
        int height);

    bool IsChecked() const;
    void SetChecked(bool checked);

    void SetEnabled(bool enabled);

    void SetOnToggle(std::function<void(bool)> callback);

private:
    virtual LRESULT HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) override;

    void Paint(HDC hdc);
    void Toggle();

    static ATOM RegisterControl(HINSTANCE instance);

private:
    bool m_checked = false;
    bool m_enabled = true;
    bool m_hovered = false;
    bool m_pressed = false;
    bool m_focused = false;
    bool m_trackingMouse = false;

    std::function<void(bool)> m_callback;

    static constexpr const wchar_t* CLASS_NAME = L"SYNKROAD_ToggleSwitch";
};