#include "MainWindow.h"

#define WM_USER_NETWORK_STATE_CHANGE (WM_USER + 101)
#define WM_USER_LATENCY_UPDATE      (WM_USER + 102)

struct NetworkStatusPayload
{
    NetworkState state;
    std::wstring message;
};

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

    m_uiManager.Create(m_window);

    m_networkManager.SetStatusCallback(
        [this](NetworkState state, const std::wstring& message)
        {
            auto* payload = new NetworkStatusPayload{ state, message };
            PostMessageW(m_window, WM_USER_NETWORK_STATE_CHANGE, 0, reinterpret_cast<LPARAM>(payload));
        });

    m_networkManager.SetLatencyCallback(
        [this](uint32_t latencyMs)
        {
            PostMessageW(m_window, WM_USER_LATENCY_UPDATE, static_cast<WPARAM>(latencyMs), 0);
        });

    m_networkManager.GetInputManager().SetLogCallback(
        [this](const std::wstring& message, bool isError)
        {
            if (isError) m_uiManager.LogError(message);
            else m_uiManager.LogInfo(message);
        });

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
    case WM_USER_NETWORK_STATE_CHANGE:
    {
        auto* payload = reinterpret_cast<NetworkStatusPayload*>(lParam);
        if (payload)
        {
            switch (payload->state)
            {
            case NetworkState::Initializing:
                m_uiManager.LogInfo(payload->message);
                break;
            case NetworkState::Ready:
                m_uiManager.LogInfo(payload->message);
                m_uiManager.SetNetworkInfo(L"Ready (" + m_networkManager.GetLocalIPAddress() + L")");
                break;
            case NetworkState::Listening:
                m_uiManager.LogInfo(payload->message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Disconnected);
                m_uiManager.SetNetworkInfo(m_networkManager.GetLocalIPAddress() + L":" + std::to_wstring(m_networkManager.GetListeningPort()));
                m_uiManager.SetLatency(0);
                break;
            case NetworkState::Connecting:
            case NetworkState::Handshaking:
                m_uiManager.LogInfo(payload->message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Connecting);
                break;
            case NetworkState::Connected:
            {
                auto dev = m_networkManager.GetConnectedDevice();
                m_uiManager.LogInfo(payload->message + L" [" + dev.deviceName + L"]");
                m_uiManager.SetConnectionStatus(ConnectionStatus::Connected);
                m_uiManager.SetNetworkInfo(dev.deviceName + L" (Session: " + std::to_wstring(dev.sessionId) + L")");
                break;
            }
            case NetworkState::Stopped:
                m_uiManager.LogInfo(payload->message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Disconnected);
                m_uiManager.SetNetworkInfo(L"No Device");
                m_uiManager.SetLatency(0);
                break;
            case NetworkState::Error:
                m_uiManager.LogError(payload->message);
                m_uiManager.SetConnectionStatus(ConnectionStatus::Disconnected);
                m_uiManager.SetNetworkInfo(L"Error");
                m_uiManager.SetLatency(0);
                break;
            default:
                m_uiManager.LogInfo(payload->message);
                break;
            }
            delete payload;
        }
        return 0;
    }

    case WM_USER_LATENCY_UPDATE:
    {
        uint32_t latencyMs = static_cast<uint32_t>(wParam);
        m_uiManager.SetLatency(latencyMs);

        if (m_networkManager.IsConnected())
        {
            auto stats = m_networkManager.GetInputManager().GetInputStatistics();
            std::wstring diag = L"Input: " + std::to_wstring(stats.packetsPerSecond) + L" PPS | Drop: " + std::to_wstring(stats.droppedPackets);
            m_uiManager.SetNetworkInfo(diag);
        }
        return 0;
    }

    case WM_SIZE:
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