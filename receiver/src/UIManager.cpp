#include "UIManager.h"

UIManager::~UIManager() = default;

ATOM UIManager::RegisterContentPanelClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return 0;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = UIManager::ContentPanelProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // Distinct soft gray background brush for content panel
    wc.hbrBackground = CreateSolidBrush(RGB(245, 246, 248));
    wc.lpszClassName = CONTENT_PANEL_CLASS;

    registered = true;
    return RegisterClassExW(&wc);
}

LRESULT CALLBACK UIManager::ContentPanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1; // Handled by class background brush

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        
        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH bgBrush = CreateSolidBrush(RGB(245, 246, 248));
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        // Draw top separator border line between header bar and content area
        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(225, 228, 232));
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
        MoveToEx(hdc, 0, 0, nullptr);
        LineTo(hdc, rc.right, 0);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool UIManager::Create(HWND parent)
{
    m_parent = parent;
    HINSTANCE instance = GetModuleHandleW(nullptr);

    // Register Content Panel Class
    RegisterContentPanelClass(instance);

    // SYNKROAD Logo Label
    m_logoLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"SYNKROAD",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        m_parent,
        nullptr,
        instance,
        nullptr);

    if (!m_logoLabel)
        return false;

    // Connection Status Indicator Label
    m_statusLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"Disconnected",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 0, 0,
        m_parent,
        nullptr,
        instance,
        nullptr);

    if (!m_statusLabel)
        return false;

    // Connect Toggle Switch
    if (!m_connectToggle.Create(
            m_parent,
            1001,
            0, 0, 0, 0))
    {
        return false;
    }

    m_connectToggle.SetChecked(false);

    m_connectToggle.SetOnToggle(
        [this](bool connected)
        {
            SetConnectionStatus(connected ? ConnectionStatus::Connected : ConnectionStatus::Disconnected);
        });

    // Content Panel Window Container
    m_contentPanel = CreateWindowExW(
        0,
        CONTENT_PANEL_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 0, 0,
        m_parent,
        nullptr,
        instance,
        nullptr);

    if (!m_contentPanel)
        return false;

    // Initial Status & Layout Calculation
    SetConnectionStatus(ConnectionStatus::Disconnected);
    UpdateLayout();

    return true;
}

void UIManager::SetConnectionStatus(ConnectionStatus status)
{
    m_currentStatus = status;

    if (!m_statusLabel)
        return;

    switch (m_currentStatus)
    {
    case ConnectionStatus::Disconnected:
        SetWindowTextW(m_statusLabel, L"Disconnected");
        break;
    case ConnectionStatus::Connecting:
        SetWindowTextW(m_statusLabel, L"Connecting...");
        break;
    case ConnectionStatus::Connected:
        SetWindowTextW(m_statusLabel, L"Connected");
        break;
    }

    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void UIManager::UpdateLayout()
{
    if (!m_parent)
        return;

    RECT clientRect{};
    GetClientRect(m_parent, &clientRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // 1. Position SYNKROAD Logo (Left-aligned)
    if (m_logoLabel)
    {
        SetWindowPos(
            m_logoLabel,
            nullptr,
            TOP_BAR_PADDING_X,
            TOP_BAR_PADDING_Y,
            LOGO_WIDTH,
            LOGO_HEIGHT,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 2. Position Connect Toggle Switch (Right-aligned)
    int toggleX = clientWidth - TOP_BAR_PADDING_X - TOGGLE_WIDTH;
    if (m_connectToggle.Handle())
    {
        SetWindowPos(
            m_connectToggle.Handle(),
            nullptr,
            toggleX,
            TOP_BAR_PADDING_Y - 2,
            TOGGLE_WIDTH,
            TOGGLE_HEIGHT,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 3. Position Status Label (Left of Toggle Switch)
    if (m_statusLabel)
    {
        int statusX = toggleX - STATUS_WIDTH - 10;
        SetWindowPos(
            m_statusLabel,
            nullptr,
            statusX,
            TOP_BAR_PADDING_Y + 3,
            STATUS_WIDTH,
            STATUS_HEIGHT,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 4. Position Content Panel (Fills full remaining client area beneath top bar)
    if (m_contentPanel)
    {
        int contentY = TOP_BAR_TOTAL_HEIGHT;
        int contentHeight = clientHeight - contentY;
        if (contentHeight < 0) contentHeight = 0;

        SetWindowPos(
            m_contentPanel,
            nullptr,
            0,
            contentY,
            clientWidth,
            contentHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}