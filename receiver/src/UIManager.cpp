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
        0, 0, 0, 0, // Positioned dynamically in UpdateLayout
        m_parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    if (!m_logoLabel)
        return false;

    // Connect Toggle Switch
    if (!m_connectToggle.Create(
            m_parent,
            1001,
            0, 0, 0, 0)) // Positioned dynamically in UpdateLayout
    {
        return false;
    }

    m_connectToggle.SetChecked(false);

    m_connectToggle.SetOnToggle(
        [](bool connected)
        {
            // Placeholder for future connection logic.
            (void)connected;
        });

    // Initial Layout Calculation
    UpdateLayout();

    return true;
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
    if (m_connectToggle.Handle())
    {
        int toggleX = clientWidth - TOP_BAR_PADDING_X - TOGGLE_WIDTH;
        if (toggleX < TOP_BAR_PADDING_X + LOGO_WIDTH + 10)
        {
            toggleX = TOP_BAR_PADDING_X + LOGO_WIDTH + 10;
        }

        SetWindowPos(
            m_connectToggle.Handle(),
            nullptr,
            toggleX,
            TOP_BAR_PADDING_Y - 2, // Minor visual adjustment to align center with logo text
            TOGGLE_WIDTH,
            TOGGLE_HEIGHT,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}