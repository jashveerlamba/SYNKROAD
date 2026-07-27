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
#include <vector>

#include "NetworkProtocol.h"
#include "InputManager.h"

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
    uint64_t lastHeartbeatTimestamp = 0;
    uint32_t lastSequenceNumber = 0;
    uint32_t lastMeasuredLatencyMs = 0;
    bool active = false;
};

class NetworkManager
{
public:
    using StatusCallback = std::function<void(NetworkState state, const std::wstring& message)>;
    using LatencyCallback = std::function<void(uint32_t latencyMs)>;

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
    void DisconnectSession(const std::wstring& reason);

    bool SendPacket(PacketType type, const uint8_t* payload, uint16_t payloadSize);
    bool ReceivePacket(const uint8_t* buffer, size_t bufferSize, sockaddr_in& fromAddr);

    void SendHeartbeat();
    void ProcessHeartbeat();
    void SendPing();
    void HandlePong(const PingPongPacket& pong);

    bool IsInitialized() const;
    bool IsListening() const;
    bool IsConnected() const;

    uint64_t GetSessionID() const;
    DeviceSession GetConnectedDevice() const;
    std::wstring GetLocalIPAddress() const;
    uint16_t GetListeningPort() const;
    NetworkState GetStatus() const;
    uint32_t GetCurrentLatency() const;

    InputManager& GetInputManager() { return m_inputManager; }
    const InputManager& GetInputManager() const { return m_inputManager; }

    void SetStatusCallback(StatusCallback callback);
    void SetLatencyCallback(LatencyCallback callback);

private:
    void SetState(NetworkState newState, const std::wstring& message);
    void ListeningWorker();
    void DiscoveryWorker();

    bool SerializePacket(PacketType type, const uint8_t* payload, uint16_t payloadSize, std::vector<uint8_t>& outBuffer);
    bool ValidatePacket(const TransportHeader& header, const uint8_t* payload, uint16_t payloadSize);

    uint64_t GenerateSecureSessionID();
    uint64_t GetCurrentTimestampMs() const;

private:
    bool m_winsockInitialized = false;
    SOCKET m_listenSocket = INVALID_SOCKET;
    SOCKET m_discoverySocket = INVALID_SOCKET;
    uint16_t m_listeningPort = 0;
    static constexpr uint16_t DISCOVERY_PORT = 9998;
    static constexpr uint64_t HEARTBEAT_TIMEOUT_MS = 5000;
    static constexpr uint64_t PING_INTERVAL_MS = 2000;

    NetworkState m_state = NetworkState::Uninitialized;
    StatusCallback m_statusCallback;
    LatencyCallback m_latencyCallback;
    mutable std::mutex m_mutex;

    std::thread m_listenThread;
    std::thread m_discoveryThread;
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_discoveryRunning{ false };

    DeviceSession m_currentSession;
    InputManager m_inputManager;
    uint32_t m_outgoingSequenceNumber = 0;
    uint32_t m_expectedSequenceNumber = 0;
    uint64_t m_lastPingSentTime = 0;
};