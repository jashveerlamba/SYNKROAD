#include "UIManager.h"

bool UIManager::Create(HWND parent)
{
    m_parent = parent;

    // SYNKROAD Logo
    m_logoLabel = CreateWindowExW(
        0,
        L"STATIC",
        L"SYNKROAD",
        WS_CHILD | WS_VISIBLE,
        20,
        15,
        220,
        30,
        m_parent,
        nullptr,
        GetModuleHandleW(nullptr),
        nullptr);

    // Connect Toggle
    if (!m_connectToggle.Create(
            m_parent,
            1001,
            700,
            12,
            70,
            35))
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

    return true;
}