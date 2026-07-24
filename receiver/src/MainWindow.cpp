#include "MainWindow.h"

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
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
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