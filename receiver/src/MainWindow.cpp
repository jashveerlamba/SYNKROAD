#include "MainWindow.h"

MainWindow::MainWindow() = default;

MainWindow::~MainWindow()
{
    m_networkManager.Shutdown();
}

bool MainWindow::Create(HINSTANCE instance, int cmdShow)
{
    m_instance = instance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = WINDOW_CLASS;

    if (!RegisterClassExW(&wc))
    {
        return false;
    }

    m_window = CreateWindowExW(
        0,
        WINDOW_CLASS,
        WINDOW_TITLE,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1024,
        728,
        nullptr,
        nullptr,
        m_instance,
        this
    );

    if (!m_window)
    {
        return false;
    }

    // Initialize UI Layout & Controls
    m_uiManager.Create(m_window);

    // Bind NetworkManager status updates to UI Manager
    m_networkManager.SetStatusCallback(
        [this](NetworkState state, const std::wstring& message)
        {
            switch (state)
            {
            case NetworkState::Initializing:
                m_uiManager.LogInfo(message);
                break;
            case NetworkState::Ready:
                m_uiManager.LogInfo(message);
                m_uiManager.SetNetworkInfo(L"Ready (" + m_networkManager.GetLocalIPAddress() + L")");
                break;
            case NetworkState::Listening:
                m_uiManager.LogInfo(message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Connected);
                m_uiManager.SetNetworkInfo(m_networkManager.GetLocalIPAddress() + L":" + std::to_wstring(m_networkManager.GetListeningPort()));
                break;
            case NetworkState::Stopped:
                m_uiManager.LogInfo(message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Disconnected);
                m_uiManager.SetNetworkInfo(L"No Device");
                break;
            case NetworkState::Error:
                m_uiManager.LogError(message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Disconnected);
                m_uiManager.SetNetworkInfo(L"Error");
                break;
            default:
                break;
            }
        });

    // Initialize Network Manager and start listening on configured port
    if (m_networkManager.Initialize())
    {
        m_networkManager.StartListening(m_configManager.GetListeningPort());
    }

    ShowWindow(m_window, cmdShow);
    UpdateWindow(m_window);

    return true;
}

int MainWindow::Run()
{
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainWindow* window = nullptr;

    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<MainWindow*>(createStruct->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (window)
    {
        return window->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        m_uiManager.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;

    case WM_DESTROY:
        m_networkManager.Shutdown();
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(m_window, message, wParam, lParam);
}