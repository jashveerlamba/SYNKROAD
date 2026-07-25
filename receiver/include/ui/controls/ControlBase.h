#pragma once

#include <windows.h>

class ControlBase
{
public:

    ControlBase() = default;
    virtual ~ControlBase() = default;

    HWND Handle() const
    {
        return m_window;
    }

protected:

    HWND m_window = nullptr;
    HWND m_parent = nullptr;

    virtual LRESULT HandleMessage(
        UINT message,
        WPARAM wParam,
        LPARAM lParam) = 0;

    static LRESULT CALLBACK WindowProc(
        HWND hwnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam)
    {
        ControlBase* control =
            reinterpret_cast<ControlBase*>(
                GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (message == WM_NCCREATE)
        {
            auto* create =
                reinterpret_cast<CREATESTRUCTW*>(lParam);

            control =
                static_cast<ControlBase*>(create->lpCreateParams);

            SetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(control));

            control->m_window = hwnd;
        }

        if (control)
        {
            return control->HandleMessage(
                message,
                wParam,
                lParam);
        }

        return DefWindowProcW(
            hwnd,
            message,
            wParam,
            lParam);
    }
};