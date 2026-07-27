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
#include <thread>
#include <atomic>
#include <cstdint>

#include "NetworkProtocol.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Bcrypt.lib")

enum class NetworkState
{
    Uninitialized,
    Initializing,
    Ready,
    Listening,
    Discovering,
    Connecting,
    Handshaking,
    Connected,
    Error,
    Stopped
};

struct DeviceSession
{
    uint64_t sessionId = 0;
    std::wstring deviceId;
    std::wstring deviceName;
    std::wstring appVersion;
    uint32_t capabilities = 0;
    sockaddr_in clientAddr{};
    uint64_t connectedTimestamp = 0;
    bool active = false;
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

    bool BeginDiscovery();
    void StopDiscovery();

    void AcceptConnection();
    void RejectConnection(HandshakeResult reason);
    void StartHandshake();
    void CompleteHandshake();
    void Disconnect();

    bool IsInitialized() const;
    bool IsListening() const;
    bool IsConnected() const;

    uint64_t GetSessionID() const;
    DeviceSession GetConnectedDevice() const;
    std::wstring GetLocalIPAddress() const;
    uint16_t GetListeningPort() const;
    NetworkState GetStatus() const;

    void SetStatusCallback(StatusCallback callback);

private:
    void SetState(NetworkState newState, const std::wstring& message);
    void ListeningWorker();
    void DiscoveryWorker();

    uint64_t GenerateSecureSessionID();
    bool ValidatePacketHeader(const PacketHeader& header, PacketType expectedType, uint16_t expectedSize);

private:
    bool m_winsockInitialized = false;
    SOCKET m_listenSocket = INVALID_SOCKET;
    SOCKET m_discoverySocket = INVALID_SOCKET;
    uint16_t m_listeningPort = 0;
    static constexpr uint16_t DISCOVERY_PORT = 9998;

    NetworkState m_state = NetworkState::Uninitialized;
    StatusCallback m_statusCallback;
    mutable std::mutex m_mutex;

    std::thread m_listenThread;
    std::thread m_discoveryThread;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_discoveryRunning{ false };

    DeviceSession m_currentSession;
};