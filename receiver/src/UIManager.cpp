#include "UIManager.h"

#include <richedit.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

UIManager::~UIManager() = default;

ATOM UIManager::RegisterContentPanelClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return 0;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = UIManager::ContentPanelProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(245, 246, 248));
    wc.lpszClassName = CONTENT_PANEL_CLASS;

    registered = true;
    return RegisterClassExW(&wc);
}

ATOM UIManager::RegisterStatusBarClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return 0;

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = UIManager::StatusBarProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(238, 240, 243));
    wc.lpszClassName = STATUS_BAR_CLASS;

    registered = true;
    return RegisterClassExW(&wc);
}

LRESULT CALLBACK UIManager::ContentPanelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);
        HBRUSH bgBrush = CreateSolidBrush(RGB(245, 246, 248));
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

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

LRESULT CALLBACK UIManager::StatusBarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH bgBrush = CreateSolidBrush(RGB(238, 240, 243));
        FillRect(hdc, &rc, bgBrush);
        DeleteObject(bgBrush);

        HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(210, 214, 220));
        HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, borderPen));
        MoveToEx(hdc, 0, 0, nullptr);
        LineTo(hdc, rc.right, 0);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(90, 95, 105));

        HFONT hFont = CreateFontW(
            -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT oldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        auto* manager = reinterpret_cast<UIManager*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

        if (manager)
        {
            const int totalWidth = rc.right - rc.left;
            const int sectionWidth = totalWidth / 5;
            const int textMarginX = 12;

            std::wstring items[5] = {
                L"Version: " + manager->m_strVersion,
                L"Status: " + manager->m_strConnState,
                L"Net: " + manager->m_strNetwork,
                L"FPS: " + manager->m_strFPS,
                L"Latency: " + manager->m_strLatency
            };

            for (int i = 0; i < 5; ++i)
            {
                int xStart = i * sectionWidth;
                int xEnd = (i == 4) ? totalWidth : (i + 1) * sectionWidth;

                RECT itemRc{ xStart + textMarginX, 0, xEnd - textMarginX, rc.bottom };
                DrawTextW(hdc, items[i].c_str(), -1, &itemRc, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

                if (i < 4)
                {
                    MoveToEx(hdc, xEnd, 4, nullptr);
                    LineTo(hdc, xEnd, rc.bottom - 4);
                }
            }
        }

        SelectObject(hdc, oldFont);
        DeleteObject(hFont);
        SelectObject(hdc, oldPen);
        DeleteObject(borderPen);

        EndPaint(hwnd, &ps);
        return 0;
    }
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

std::wstring UIManager::GetFormattedTimestamp() const
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::tm buf{};
    localtime_s(&buf, &in_time_t);

    std::wstringstream ss;
    ss << std::setfill(L'0') 
       << std::setw(2) << buf.tm_hour << L":"
       << std::setw(2) << buf.tm_min << L":"
       << std::setw(2) << buf.tm_sec;

    return ss.str();
}

void UIManager::AppendLogToRichEdit(const LogEntry& entry)
{
    if (!m_logRichEdit) return;

    // Move insertion to the end
    CHARRANGE cr{ -1, -1 };
    SendMessageW(m_logRichEdit, EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&cr));

    // Determine Prefix text and Color
    std::wstring levelStr;
    COLORREF color = RGB(30, 30, 30);

    switch (entry.level)
    {
    case LogLevel::Debug:
        levelStr = L"DEBUG";
        color = RGB(120, 120, 120); // Gray
        break;
    case LogLevel::Info:
        levelStr = L"INFO ";
        color = RGB(33, 115, 70);   // Dark Green
        break;
    case LogLevel::Warning:
        levelStr = L"WARN ";
        color = RGB(217, 119, 6);   // Dark Amber/Orange
        break;
    case LogLevel::Error:
        levelStr = L"ERROR";
        color = RGB(220, 38, 38);   // Red
        break;
    }

    std::wstring line = L"[" + entry.timestamp + L"] " + levelStr + L"  " + entry.message + L"\r\n";

    CHARFORMAT2W cf{};
    cf.cbSize = sizeof(CHARFORMAT2W);
    cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
    cf.crTextColor = color;
    cf.yHeight = 180; // 9pt
    wcscpy_s(cf.szFaceName, L"Consolas");

    SendMessageW(m_logRichEdit, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));
    SendMessageW(m_logRichEdit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(line.c_str()));

    // Auto-scroll to the bottom
    SendMessageW(m_logRichEdit, WM_VSCROLL, SB_BOTTOM, 0);
}

bool UIManager::Create(HWND parent)
{
    m_parent = parent;
    HINSTANCE instance = GetModuleHandleW(nullptr);

    // Load Msftedit.dll / RichEdit v4.1 or fallback to RichEd20.dll
    LoadLibraryW(L"Msftedit.dll");

    RegisterContentPanelClass(instance);
    RegisterStatusBarClass(instance);

    // SYNKROAD Logo Label
    m_logoLabel = CreateWindowExW(
        0, L"STATIC", L"SYNKROAD",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_parent, nullptr, instance, nullptr);

    if (!m_logoLabel) return false;

    // Connection Status Indicator Label
    m_statusLabel = CreateWindowExW(
        0, L"STATIC", L"Disconnected",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        0, 0, 0, 0, m_parent, nullptr, instance, nullptr);

    if (!m_statusLabel) return false;

    // Connect Toggle Switch
    if (!m_connectToggle.Create(m_parent, 1001, 0, 0, 0, 0))
    {
        return false;
    }

    m_connectToggle.SetChecked(false);
    m_connectToggle.SetOnToggle(
        [this](bool connected)
        {
            SetConnectionStatus(connected ? ConnectionStatus::Connected : ConnectionStatus::Disconnected);
        });

    // Content Panel Container
    m_contentPanel = CreateWindowExW(
        0, CONTENT_PANEL_CLASS, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, 0, 0, m_parent, nullptr, instance, nullptr);

    if (!m_contentPanel) return false;

    // Log Control (RichEdit inside Content Panel)
    m_logRichEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        MSFTEDIT_CLASS,
        L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        0, 0, 0, 0,
        m_contentPanel,
        nullptr,
        instance,
        nullptr);

    if (!m_logRichEdit)
    {
        // Fallback to RichEdit20W if Msftedit isn't available
        m_logRichEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"RichEdit20W",
            L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            0, 0, 0, 0,
            m_contentPanel,
            nullptr,
            instance,
            nullptr);
    }

    if (m_logRichEdit)
    {
        // Set background color to crisp off-white
        SendMessageW(m_logRichEdit, EM_SETBKGNDCOLOR, 0, RGB(252, 252, 253));
    }

    // Status Bar Window Container
    m_statusBar = CreateWindowExW(
        0, STATUS_BAR_CLASS, L"",
        WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0, m_parent, nullptr, instance, nullptr);

    if (!m_statusBar) return false;

    SetWindowLongPtrW(m_statusBar, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    SetConnectionStatus(ConnectionStatus::Disconnected);
    UpdateLayout();

    // Initial startup log message
    LogInfo(L"Receiver started");

    return true;
}

void UIManager::SetConnectionStatus(ConnectionStatus status)
{
    m_currentStatus = status;

    if (!m_statusLabel) return;

    switch (m_currentStatus)
    {
    case ConnectionStatus::Disconnected:
        SetWindowTextW(m_statusLabel, L"Disconnected");
        SetConnectionStateText(L"Disconnected");
        LogInfo(L"Connection state changed: Disconnected");
        break;
    case ConnectionStatus::Connecting:
        SetWindowTextW(m_statusLabel, L"Connecting...");
        SetConnectionStateText(L"Connecting");
        LogInfo(L"Connection state changed: Connecting...");
        break;
    case ConnectionStatus::Connected:
        SetWindowTextW(m_statusLabel, L"Connected");
        SetConnectionStateText(L"Connected");
        LogInfo(L"Connection state changed: Connected");
        break;
    }

    InvalidateRect(m_statusLabel, nullptr, TRUE);
}

void UIManager::SetVersion(const std::wstring& version)
{
    m_strVersion = version;
    if (m_statusBar) InvalidateRect(m_statusBar, nullptr, TRUE);
}

void UIManager::SetConnectionStateText(const std::wstring& state)
{
    m_strConnState = state;
    if (m_statusBar) InvalidateRect(m_statusBar, nullptr, TRUE);
}

void UIManager::SetNetworkInfo(const std::wstring& info)
{
    m_strNetwork = info;
    if (m_statusBar) InvalidateRect(m_statusBar, nullptr, TRUE);
}

void UIManager::SetFPS(int fps)
{
    m_strFPS = std::to_wstring(fps) + L" FPS";
    if (m_statusBar) InvalidateRect(m_statusBar, nullptr, TRUE);
}

void UIManager::SetLatency(int latencyMs)
{
    m_strLatency = (latencyMs < 0) ? L"-- ms" : (std::to_wstring(latencyMs) + L" ms");
    if (m_statusBar) InvalidateRect(m_statusBar, nullptr, TRUE);
}

void UIManager::LogInfo(const std::wstring& message)
{
    Log(LogLevel::Info, message);
}

void UIManager::LogWarning(const std::wstring& message)
{
    Log(LogLevel::Warning, message);
}

void UIManager::LogError(const std::wstring& message)
{
    Log(LogLevel::Error, message);
}

void UIManager::LogDebug(const std::wstring& message)
{
#if defined(_DEBUG) || !defined(NDEBUG)
    Log(LogLevel::Debug, message);
#else
    (void)message;
#endif
}

void UIManager::Log(LogLevel level, const std::wstring& message)
{
    LogEntry entry{ level, GetFormattedTimestamp(), message };

    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_logs.push_back(entry);
    }

    AppendLogToRichEdit(entry);
}

void UIManager::ClearLog()
{
    {
        std::lock_guard<std::mutex> lock(m_logMutex);
        m_logs.clear();
    }

    if (m_logRichEdit)
    {
        SetWindowTextW(m_logRichEdit, L"");
    }
}

void UIManager::UpdateLayout()
{
    if (!m_parent) return;

    RECT clientRect{};
    GetClientRect(m_parent, &clientRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // 1. Position SYNKROAD Logo
    if (m_logoLabel)
    {
        SetWindowPos(m_logoLabel, nullptr, TOP_BAR_PADDING_X, TOP_BAR_PADDING_Y, LOGO_WIDTH, LOGO_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 2. Position Connect Toggle Switch
    int toggleX = clientWidth - TOP_BAR_PADDING_X - TOGGLE_WIDTH;
    if (m_connectToggle.Handle())
    {
        SetWindowPos(m_connectToggle.Handle(), nullptr, toggleX, TOP_BAR_PADDING_Y - 2, TOGGLE_WIDTH, TOGGLE_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 3. Position Status Label
    if (m_statusLabel)
    {
        int statusX = toggleX - STATUS_WIDTH - 10;
        SetWindowPos(m_statusLabel, nullptr, statusX, TOP_BAR_PADDING_Y + 3, STATUS_WIDTH, STATUS_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 4. Position Bottom Status Bar
    int statusBarY = clientHeight - STATUS_BAR_HEIGHT;
    if (statusBarY < TOP_BAR_TOTAL_HEIGHT) statusBarY = TOP_BAR_TOTAL_HEIGHT;

    if (m_statusBar)
    {
        SetWindowPos(m_statusBar, nullptr, 0, statusBarY, clientWidth, STATUS_BAR_HEIGHT, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 5. Position Central Main Content Panel
    int contentY = TOP_BAR_TOTAL_HEIGHT;
    int contentHeight = statusBarY - contentY;
    if (contentHeight < 0) contentHeight = 0;

    if (m_contentPanel)
    {
        SetWindowPos(m_contentPanel, nullptr, 0, contentY, clientWidth, contentHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // 6. Position Logging Control within Content Panel
    if (m_logRichEdit)
    {
        int logWidth = clientWidth - (2 * CONTENT_MARGIN);
        int logHeight = contentHeight - (2 * CONTENT_MARGIN);

        if (logWidth < 0) logWidth = 0;
        if (logHeight < 0) logHeight = 0;

        SetWindowPos(
            m_logRichEdit,
            nullptr,
            CONTENT_MARGIN,
            CONTENT_MARGIN,
            logWidth,
            logHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}