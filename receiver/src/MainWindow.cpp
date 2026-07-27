#include "MainWindow.h"
#include <windowsx.h>
#include <commctrl.h>

MainWindow::MainWindow() : m_hwnd(NULL)
{
}

MainWindow::~MainWindow()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
    }
}

bool MainWindow::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEX wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"SYNKROADReceiverWindowClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    m_hwnd = CreateWindowEx(
        0,
        L"SYNKROADReceiverWindowClass",
        L"SYNKROAD Receiver",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        NULL,
        NULL,
        hInstance,
        this
    );

    if (!m_hwnd)
    {
        return false;
    }

    ShowWindow(m_hwnd, nCmdShow);
    UpdateWindow(m_hwnd);

    return true;
}

void MainWindow::RunMessageLoop()
{
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    MainWindow* pThis = NULL;

    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (MainWindow*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->m_hwnd = hwnd;
    }
    else
    {
        pThis = (MainWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis)
    {
        return pThis->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT MainWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Load initial config
        m_configManager.LoadConfig();
        
        // Initialize UI components
        if (!m_uiManager.Initialize(m_hwnd, m_configManager.GetConfig()))
        {
            MessageBox(m_hwnd, L"Failed to initialize UI Manager.", L"Error", MB_ICONERROR);
            return -1;
        }

        // Set up Network Logging Callback
        m_networkManager.SetLogCallback([this](const std::wstring& msg, bool isError) {
            m_uiManager.AppendLog(msg, isError);
        });

        // Set up Input Manager & Injection Framework Callback
        m_networkManager.GetInputManager().SetLogCallback([this](const std::wstring& msg, bool isError) {
            m_uiManager.AppendLog(msg, isError);
        });

        // Register window handle to receive custom network notifications
        m_networkManager.SetNotificationWindow(m_hwnd);

        // Auto-start listener service
        if (!m_networkManager.Start(m_configManager.GetConfig().port))
        {
            m_uiManager.AppendLog(L"Failed to start Network Receiver on configured port.", true);
        }

        return 0;
    }

    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        m_uiManager.Resize(width, height);
        return 0;
    }

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);

        // Handle UI control interactions
        m_uiManager.HandleCommand(wmId, wmEvent, (HWND)lParam);
        return 0;
    }

    case WM_USER_CONNECTION_CHANGED:
    {
        bool connected = (wParam != 0);
        m_uiManager.UpdateConnectionStatus(connected);

        if (connected)
        {
            m_uiManager.AppendLog(L"Client established connection successfully.");
        }
        else
        {
            m_uiManager.AppendLog(L"Client disconnected. Resetting injection pipeline...");
            // Safely release active injection states on disconnection
            m_networkManager.GetInputManager().ResetPipeline();
            m_uiManager.SetNetworkInfo(L"Disconnected");
        }
        return 0;
    }

    case WM_USER_LATENCY_UPDATE:
    {
        uint32_t latencyMs = static_cast<uint32_t>(wParam);
        m_uiManager.SetLatency(latencyMs);

        if (m_networkManager.IsConnected())
        {
            auto& inputMgr = m_networkManager.GetInputManager();
            auto inputStats = inputMgr.GetInputStatistics();
            auto injectStats = inputMgr.GetInjectionManager().GetInjectionStatistics();

            std::wstring diag = L"PPS: " + std::to_wstring(inputStats.packetsPerSecond) + 
                                L" | Injected Events: " + std::to_wstring(injectStats.totalButtonEvents + injectStats.totalAxisUpdates) +
                                L" | Latency: " + std::to_wstring(injectStats.averageLatencyUs) + L"us";
            
            m_uiManager.SetNetworkInfo(diag);
        }
        return 0;
    }

    case WM_DESTROY:
    {
        // Graceful shutdown order: Stop Network -> Stop Injection -> Persist Config -> Terminate UI
        m_networkManager.Stop();
        m_networkManager.GetInputManager().GetInjectionManager().Shutdown();
        m_configManager.SaveConfig();
        m_uiManager.Shutdown();
        
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(m_hwnd, message, wParam, lParam);
    }
}