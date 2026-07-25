#include "UIManager.h"

bool UIManager::Create(HWND parent)
{
    m_parent = parent;

    // SYNKROAD Logo Label
    m_logoLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"SYNKROAD",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        m_parent,
        nullptr,
        GetModuleHandleW(nullptr),
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
        GetModuleHandleW(nullptr),
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

    // Wire up toggle switch to demo status changes
    m_connectToggle.SetOnToggle(
        [this](bool connected)
        {
            SetConnectionStatus(connected ? ConnectionStatus::Connected : ConnectionStatus::Disconnected);
        });

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
}