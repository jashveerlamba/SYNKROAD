#include "ui/controls/ToggleSwitch.h"

ToggleSwitch::ToggleSwitch()
{
}

ToggleSwitch::~ToggleSwitch()
{
}

ATOM ToggleSwitch::RegisterControl(HINSTANCE instance)
{
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ControlBase::WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr; // Handled in double-buffering Paint to prevent flicker
    wc.lpszClassName = CLASS_NAME;

    return RegisterClassExW(&wc);
}

bool ToggleSwitch::Create(
    HWND parent,
    int id,
    int x,
    int y,
    int width,
    int height)
{
    HINSTANCE instance = GetModuleHandleW(nullptr);
    RegisterControl(instance);

    m_hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        instance,
        this);

    return m_hwnd != nullptr;
}

bool ToggleSwitch::IsChecked() const
{
    return m_checked;
}

void SetCheckedInternal(ToggleSwitch* control, bool checked, bool triggerCallback)
{
    // Utility check
}

void ToggleSwitch::SetChecked(bool checked)
{
    if (m_checked != checked)
    {
        m_checked = checked;
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void ToggleSwitch::SetEnabled(bool enabled)
{
    if (m_enabled != enabled)
    {
        m_enabled = enabled;
        EnableWindow(m_hwnd, enabled ? TRUE : FALSE);
        InvalidateRect(m_hwnd, nullptr, FALSE);
    }
}

void ToggleSwitch::SetOnToggle(std::function<void(bool)> callback)
{
    m_callback = std::move(callback);
}

void ToggleSwitch::Toggle()
{
    if (!m_enabled) return;

    m_checked = !m_checked;
    InvalidateRect(m_hwnd, nullptr, FALSE);

    if (m_callback)
    {
        m_callback(m_checked);
    }
}

LRESULT ToggleSwitch::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        // Prevent flicker by handling drawing entirely in WM_PAINT
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        Paint(hdc);
        EndPaint(m_hwnd, &ps);
        return 0;
    }

    case WM_MOUSEMOVE:
    {
        if (!m_trackingMouse)
        {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT) };
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_hwnd;
            TrackMouseEvent(&tme);
            m_trackingMouse = true;
        }

        if (!m_hovered)
        {
            m_hovered = true;
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_MOUSELEAVE:
    {
        m_trackingMouse = false;
        m_hovered = false;
        m_pressed = false;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        if (m_enabled)
        {
            SetFocus(m_hwnd);
            m_pressed = true;
            SetCapture(m_hwnd);
            InvalidateRect(m_hwnd, nullptr, FALSE);
        }
        return 0;
    }

    case WM_LBUTTONUP:
    {
        if (m_pressed)
        {
            ReleaseCapture();
            m_pressed = false;

            RECT rc;
            GetClientRect(m_hwnd, &rc);
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            if (PtInRect(&rc, pt))
            {
                Toggle();
            }
            else
            {
                InvalidateRect(m_hwnd, nullptr, FALSE);
            }
        }
        return 0;
    }

    case WM_SETFOCUS:
    {
        m_focused = true;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KILLFOCUS:
    {
        m_focused = false;
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }

    case WM_KEYDOWN:
    {
        if (wParam == VK_SPACE && m_enabled)
        {
            m_pressed = true;
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }

    case WM_KEYUP:
    {
        if (wParam == VK_SPACE && m_enabled)
        {
            if (m_pressed)
            {
                m_pressed = false;
                Toggle();
            }
            return 0;
        }
        break;
    }

    case WM_ENABLE:
    {
        m_enabled = (wParam != FALSE);
        InvalidateRect(m_hwnd, nullptr, FALSE);
        return 0;
    }
    }

    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

void ToggleSwitch::Paint(HDC hdc)
{
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (width <= 0 || height <= 0) return;

    // Double buffering setup
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, memBitmap));

    // Determine parent background color
    HWND parentHwnd = GetParent(m_hwnd);
    HBRUSH bgBrush = reinterpret_cast<HBRUSH>(GetClassLongPtrW(parentHwnd, GCLP_HBRBACKGROUND));
    if (!bgBrush)
    {
        bgBrush = static_cast<HBRUSH>(GetStockObject(COLOR_WINDOW + 1));
    }
    FillRect(memDC, &clientRect, bgBrush);

    // Color definitions
    COLORREF trackColor;
    COLORREF thumbColor = RGB(255, 255, 255);
    COLORREF borderColor = RGB(160, 160, 160);

    if (!m_enabled)
    {
        trackColor = m_checked ? RGB(180, 205, 230) : RGB(220, 220, 220);
        thumbColor = RGB(240, 240, 240);
        borderColor = RGB(200, 200, 200);
    }
    else if (m_checked)
    {
        if (m_pressed) trackColor = RGB(0, 90, 158);
        else if (m_hovered) trackColor = RGB(0, 108, 190);
        else trackColor = RGB(0, 120, 212); // Modern Windows accent blue
        borderColor = trackColor;
    }
    else
    {
        if (m_pressed) trackColor = RGB(210, 210, 210);
        else if (m_hovered) trackColor = RGB(230, 230, 230);
        else trackColor = RGB(245, 245, 245);
    }

    // Geometry calculation for the track
    int trackHeight = (height * 3) / 4;
    int trackWidth = width - 32; // Reserve right side for status text
    if (trackWidth < trackHeight * 2) trackWidth = trackHeight * 2;

    int trackX = 2;
    int trackY = (height - trackHeight) / 2;

    // Draw focus indicator ring if focused
    if (m_focused && m_enabled)
    {
        HPEN focusPen = CreatePen(PS_DOT, 1, RGB(0, 120, 212));
        HPEN oldPen = static_cast<HPEN>(SelectObject(memDC, focusPen));
        HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(NULL_BRUSH)));

        RoundRect(memDC, trackX - 2, trackY - 2, trackX + trackWidth + 2, trackY + trackHeight + 2, trackHeight + 4, trackHeight + 4);

        SelectObject(memDC, oldPen);
        SelectObject(memDC, oldBrush);
        DeleteObject(focusPen);
    }

    // Draw Track
    HPEN trackPen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH trackBrush = CreateSolidBrush(trackColor);

    HPEN oldPen = static_cast<HPEN>(SelectObject(memDC, trackPen));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(memDC, trackBrush));

    RoundRect(memDC, trackX, trackY, trackX + trackWidth, trackY + trackHeight, trackHeight, trackHeight);

    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);
    DeleteObject(trackPen);
    DeleteObject(trackBrush);

    // Draw Thumb
    int margin = 3;
    int thumbDiameter = trackHeight - (margin * 2);
    int thumbMinX = trackX + margin;
    int thumbMaxX = trackX + trackWidth - margin - thumbDiameter;
    int thumbX = m_checked ? thumbMaxX : thumbMinX;
    int thumbY = trackY + margin;

    HPEN thumbPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HBRUSH thumbBrush = CreateSolidBrush(thumbColor);

    oldPen = static_cast<HPEN>(SelectObject(memDC, thumbPen));
    oldBrush = static_cast<HBRUSH>(SelectObject(memDC, thumbBrush));

    Ellipse(memDC, thumbX, thumbY, thumbX + thumbDiameter, thumbY + thumbDiameter);

    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBrush);
    DeleteObject(thumbPen);
    DeleteObject(thumbBrush);

    // Draw ON/OFF Text Indicator
    const wchar_t* statusText = m_checked ? L"ON" : L"OFF";
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, m_enabled ? (m_checked ? RGB(0, 120, 212) : RGB(100, 100, 100)) : RGB(160, 160, 160));

    HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, font));

    RECT textRect = { trackX + trackWidth + 8, 0, width, height };
    DrawTextW(memDC, statusText, -1, &textRect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    SelectObject(memDC, oldFont);

    // Transfer off-screen buffer to screen
    BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}