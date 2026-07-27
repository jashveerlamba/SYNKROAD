#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <string>
#include <functional>
#include <mutex>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

enum class NetworkState
{
    Uninitialized,
    Initializing,
    Ready,
    Listening,
    Error,
    Stopped
};

class NetworkManager
{
public:
    using StatusCallback = std::function<void(NetworkState state, const std::wstring& message)>;

    NetworkManager() = default;
    ~NetworkManager();

    bool Initialize();
    void Shutdown();

    bool StartListening(uint16_t port);
    void StopListening();

    bool IsInitialized() const;
    bool IsListening() const;

    std::wstring GetLocalIPAddress() const;
    uint16_t GetListeningPort() const;
    NetworkState GetStatus() const;

    void SetStatusCallback(StatusCallback callback);

private:
    void SetState(NetworkState newState, const std::wstring& message);

private:
    bool m_winsockInitialized = false;
    SOCKET m_listenSocket = INVALID_SOCKET;
    uint16_t m_listeningPort = 0;
    NetworkState m_state = NetworkState::Uninitialized;

    StatusCallback m_statusCallback;
    mutable std::mutex m_mutex;
};