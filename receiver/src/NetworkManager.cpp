#include "NetworkManager.h"

NetworkManager::~NetworkManager()
{
    Shutdown();
}

void NetworkManager::SetStatusCallback(StatusCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusCallback = std::move(callback);
}

void NetworkManager::SetState(NetworkState newState, const std::wstring& message)
{
    StatusCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = newState;
        cb = m_statusCallback;
    }

    if (cb)
    {
        cb(newState, message);
    }
}

bool NetworkManager::Initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_winsockInitialized)
    {
        return true;
    }

    SetState(NetworkState::Initializing, L"Initializing Winsock...");

    WSADATA wsaData{};
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        SetState(NetworkState::Error, L"Network initialization failed (WSAStartup error " + std::to_wstring(result) + L")");
        return false;
    }

    m_winsockInitialized = true;
    SetState(NetworkState::Ready, L"Winsock initialized successfully");
    return true;
}

void NetworkManager::Shutdown()
{
    StopListening();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_winsockInitialized)
    {
        WSACleanup();
        m_winsockInitialized = false;
        SetState(NetworkState::Uninitialized, L"Receiver network engine shut down");
    }
}

bool NetworkManager::StartListening(uint16_t port)
{
    if (!IsInitialized())
    {
        if (!Initialize())
        {
            return false;
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_listenSocket != INVALID_SOCKET)
    {
        return true;
    }

    m_listenSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        SetState(NetworkState::Error, L"Failed to create UDP socket (Error " + std::to_wstring(err) + L")");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        SetState(NetworkState::Error, L"Failed to bind socket to port " + std::to_wstring(port) + L" (Error " + std::to_wstring(err) + L")");
        return false;
    }

    m_listeningPort = port;
    SetState(NetworkState::Listening, L"Listening on port " + std::to_wstring(port));
    return true;
}

void NetworkManager::StopListening()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        m_listeningPort = 0;
        SetState(NetworkState::Stopped, L"Receiver stopped listening");
    }
}

bool NetworkManager::IsInitialized() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_winsockInitialized;
}

bool NetworkManager::IsListening() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_listenSocket != INVALID_SOCKET;
}

uint16_t NetworkManager::GetListeningPort() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_listeningPort;
}

NetworkState NetworkManager::GetStatus() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

std::wstring NetworkManager::GetLocalIPAddress() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_winsockInitialized)
    {
        return L"127.0.0.1";
    }

    char hostName[256] = { 0 };
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
    {
        return L"127.0.0.1";
    }

    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostName, nullptr, &hints, &res) != 0 || !res)
    {
        return L"127.0.0.1";
    }

    auto* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    char ipStr[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);

    freeaddrinfo(res);

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, NULL, 0);
    std::wstring wstr(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, &wstr[0], size_needed);

    return wstr;
}