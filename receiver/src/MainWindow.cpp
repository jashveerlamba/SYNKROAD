#include "MainWindow.h"
#include <string>

MainWindow::MainWindow() = default;

MainWindow::~MainWindow() = default;

bool MainWindow::Create(HINSTANCE instance, int cmdShow)
{
    m_instance = instance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS;

    if (!RegisterClassExW(&wc))
        return false;

    m_window = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        650,
        nullptr,
        nullptr,
        m_instance,
        this);

    if (!m_window)
        return false;
    
    m_uiManager.Create(m_window);
    ShowWindow(m_window, cmdShow);
    UpdateWindow(m_window);

    return true;
}

int MainWindow::Run()
{
    MSG msg{};

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    MainWindow* window = nullptr;

    if (message == WM_NCCREATE)
    {
        auto* create =
            reinterpret_cast<CREATESTRUCTW*>(lParam);

        window =
            static_cast<MainWindow*>(create->lpCreateParams);

        SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(window));

        window->m_window = hwnd;
    }
    else
    {
        window = reinterpret_cast<MainWindow*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window)
        return window->HandleMessage(message, wParam, lParam);

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED)
        {
            m_uiManager.UpdateLayout();
        }
        return 0;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = reinterpret_cast<HDC>(wParam);
        HWND hwndStatic = reinterpret_cast<HWND>(lParam);

        // Retrieve current text in status label to determine state color
        wchar_t text[64] = { 0 };
        GetWindowTextW(hwndStatic, text, 64);

        if (wcscmp(text, L"Connected") == 0)
        {
            SetTextColor(hdcStatic, RGB(40, 167, 69)); // Green
        }
        else if (wcscmp(text, L"Connecting...") == 0)
        {
            SetTextColor(hdcStatic, RGB(255, 140, 0)); // Orange
        }
        else if (wcscmp(text, L"Disconnected") == 0)
        {
            SetTextColor(hdcStatic, RGB(128, 128, 128)); // Gray
        }

        SetBkMode(hdcStatic, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(GetSysColorBrush(COLOR_WINDOW));
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(
        m_window,
        message,
        wParam,
        lParam);
}