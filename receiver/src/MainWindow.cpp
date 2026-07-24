#include "MainWindow.h"

MainWindow::MainWindow()
    : m_window(nullptr),
      m_instance(nullptr)
{
}

MainWindow::~MainWindow()
{
}

bool MainWindow::Create(HINSTANCE instance, int cmdShow)
{
    m_instance = instance;

    const wchar_t CLASS_NAME[] = L"SYNKROAD_MainWindow";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    RegisterClassExW(&wc);

    m_window = CreateWindowExW(
        0,
        CLASS_NAME,
        L"SYNKROAD Receiver",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        900,
        600,
        nullptr,
        nullptr,
        m_instance,
        this);

    if (!m_window)
        return false;

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
        CREATESTRUCT* create =
            reinterpret_cast<CREATESTRUCT*>(lParam);

        window =
            static_cast<MainWindow*>(create->lpCreateParams);

        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(window));

        window->m_window = hwnd;
    }
    else
    {
        window = reinterpret_cast<MainWindow*>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (window)
    {
        return window->HandleMessage(
            message,
            wParam,
            lParam);
    }

    return DefWindowProc(
        hwnd,
        message,
        wParam,
        lParam);
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

    return DefWindowProc(
        m_window,
        message,
        wParam,
        lParam);
}