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

    // Connect Button (temporary)
    m_connectButton = CreateWindowExW(
        0,
        L"BUTTON",
        L"Connect",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        700,
        12,
        150,
        35,
        m_parent,
        reinterpret_cast<HMENU>(1001),
        GetModuleHandleW(nullptr),
        nullptr);

    return true;
}