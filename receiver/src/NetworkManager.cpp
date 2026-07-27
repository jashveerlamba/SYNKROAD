#include "NetworkManager.h"
#include <bcrypt.h>
#include <chrono>
#include <algorithm>

NetworkManager::~NetworkManager()
{
    Shutdown();
}

void NetworkManager::SetStatusCallback(StatusCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_statusCallback = std::move(callback);
}

void NetworkManager::SetLatencyCallback(LatencyCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_latencyCallback = std::move(callback);
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

uint64_t NetworkManager::GetCurrentTimestampMs() const
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
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
    StopDiscovery();
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
        SetState(NetworkState::Error, L"Failed to create socket (Error " + std::to_wstring(err) + L")");
        return false;
    }

    u_long mode = 1;
    ioctlsocket(m_listenSocket, FIONBIO, &mode);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        int err = WSAGetLastError();
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        SetState(NetworkState::Error, L"Failed to bind socket (Error " + std::to_wstring(err) + L")");
        return false;
    }

    m_listeningPort = port;
    m_running = true;
    m_listenThread = std::thread(&NetworkManager::ListeningWorker, this);

    BeginDiscovery();

    SetState(NetworkState::Listening, L"Receiver listening on port " + std::to_wstring(port));
    return true;
}

void NetworkManager::StopListening()
{
    m_running = false;
    if (m_listenThread.joinable())
    {
        m_listenThread.join();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_listenSocket != INVALID_SOCKET)
    {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        m_listeningPort = 0;
        m_currentSession = {};
        SetState(NetworkState::Stopped, L"Receiver stopped listening");
    }
}

bool NetworkManager::BeginDiscovery()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_discoverySocket != INVALID_SOCKET)
    {
        return true;
    }

    m_discoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_discoverySocket == INVALID_SOCKET)
    {
        return false;
    }

    BOOL optval = TRUE;
    setsockopt(m_discoverySocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optval), sizeof(optval));

    u_long mode = 1;
    ioctlsocket(m_discoverySocket, FIONBIO, &mode);

    sockaddr_in discAddr{};
    discAddr.sin_family = AF_INET;
    discAddr.sin_addr.s_addr = INADDR_ANY;
    discAddr.sin_port = htons(DISCOVERY_PORT);

    if (bind(m_discoverySocket, reinterpret_cast<sockaddr*>(&discAddr), sizeof(discAddr)) == SOCKET_ERROR)
    {
        closesocket(m_discoverySocket);
        m_discoverySocket = INVALID_SOCKET;
        return false;
    }

    m_discoveryRunning = true;
    m_discoveryThread = std::thread(&NetworkManager::DiscoveryWorker, this);
    return true;
}

void NetworkManager::StopDiscovery()
{
    m_discoveryRunning = false;
    if (m_discoveryThread.joinable())
    {
        m_discoveryThread.join();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_discoverySocket != INVALID_SOCKET)
    {
        closesocket(m_discoverySocket);
        m_discoverySocket = INVALID_SOCKET;
    }
}

void NetworkManager::DiscoveryWorker()
{
    char buffer[512];
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);

    while (m_discoveryRunning)
    {
        int bytesRecv = recvfrom(m_discoverySocket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (bytesRecv > 0 && static_cast<size_t>(bytesRecv) >= sizeof(DiscoveryRequestPacket))
        {
            auto* req = reinterpret_cast<DiscoveryRequestPacket*>(buffer);
            if (ValidatePacket(req->header, reinterpret_cast<const uint8_t*>(buffer) + sizeof(TransportHeader), req->header.payloadLength))
            {
                SetState(GetStatus(), L"Android device discovered on network");

                DiscoveryResponsePacket resp{};
                resp.header.magic = SYNKROAD_MAGIC;
                resp.header.protocolVersion = CURRENT_PROTOCOL_VERSION;
                resp.header.type = PacketType::DiscoveryResponse;
                resp.header.sessionId = 0;
                resp.header.sequenceNumber = 0;
                resp.header.timestamp = GetCurrentTimestampMs();
                resp.header.payloadLength = sizeof(DiscoveryResponsePacket) - sizeof(TransportHeader);
                
                strcpy_s(resp.receiverName, "SYNKROAD Receiver");
                resp.listeningPort = m_listeningPort;

                resp.header.crc32 = NetworkUtils::CalculateCRC32(
                    reinterpret_cast<const uint8_t*>(&resp) + offsetof(TransportHeader, payloadLength),
                    sizeof(DiscoveryResponsePacket) - offsetof(TransportHeader, payloadLength)
                );

                sendto(m_discoverySocket, reinterpret_cast<const char*>(&resp), sizeof(resp), 0,
                       reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

bool NetworkManager::SerializePacket(PacketType type, const uint8_t* payload, uint16_t payloadSize, std::vector<uint8_t>& outBuffer)
{
    size_t totalSize = sizeof(TransportHeader) + payloadSize;
    if (totalSize > MAX_PACKET_SIZE) return false;

    outBuffer.resize(totalSize);
    auto* header = reinterpret_cast<TransportHeader*>(outBuffer.data());
    header->magic = SYNKROAD_MAGIC;
    header->protocolVersion = CURRENT_PROTOCOL_VERSION;
    header->type = type;
    header->sessionId = m_currentSession.sessionId;
    header->sequenceNumber = ++m_outgoingSequenceNumber;
    header->timestamp = GetCurrentTimestampMs();
    header->payloadLength = payloadSize;

    if (payload && payloadSize > 0)
    {
        std::copy(payload, payload + payloadSize, outBuffer.data() + sizeof(TransportHeader));
    }

    header->crc32 = NetworkUtils::CalculateCRC32(
        outBuffer.data() + offsetof(TransportHeader, payloadLength),
        totalSize - offsetof(TransportHeader, payloadLength)
    );

    return true;
}

bool NetworkManager::ValidatePacket(const TransportHeader& header, const uint8_t* payload, uint16_t payloadSize)
{
    if (header.magic != SYNKROAD_MAGIC) return false;
    if (header.protocolVersion != CURRENT_PROTOCOL_VERSION) return false;
    if (header.payloadLength != payloadSize) return false;

    std::vector<uint8_t> checkBuf(sizeof(uint16_t) + payloadSize);
    *reinterpret_cast<uint16_t*>(checkBuf.data()) = header.payloadLength;
    if (payload && payloadSize > 0)
    {
        std::copy(payload, payload + payloadSize, checkBuf.data() + sizeof(uint16_t));
    }

    uint32_t computedCrc = NetworkUtils::CalculateCRC32(checkBuf.data(), checkBuf.size());
    return computedCrc == header.crc32;
}

bool NetworkManager::SendPacket(PacketType type, const uint8_t* payload, uint16_t payloadSize)
{
    std::vector<uint8_t> packetData;
    if (!SerializePacket(type, payload, payloadSize, packetData)) return false;

    int sent = sendto(m_listenSocket, reinterpret_cast<const char*>(packetData.data()),
                      static_cast<int>(packetData.size()), 0,
                      reinterpret_cast<const sockaddr*>(&m_currentSession.clientAddr),
                      sizeof(m_currentSession.clientAddr));
    return sent != SOCKET_ERROR;
}

void NetworkManager::ListeningWorker()
{
    std::vector<uint8_t> buffer(MAX_PACKET_SIZE);
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);
    uint64_t lastPingCheckTime = 0;

    while (m_running)
    {
        int bytesRecv = recvfrom(m_listenSocket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        uint64_t now = GetCurrentTimestampMs();

        if (bytesRecv > 0 && static_cast<size_t>(bytesRecv) >= sizeof(TransportHeader))
        {
            auto* header = reinterpret_cast<TransportHeader*>(buffer.data());
            const uint8_t* payload = buffer.data() + sizeof(TransportHeader);
            uint16_t payloadSize = static_cast<uint16_t>(bytesRecv - sizeof(TransportHeader));

            if (ValidatePacket(*header, payload, payloadSize))
            {
                if (header->type == PacketType::HandshakeRequest)
                {
                    SetState(NetworkState::Connecting, L"Connection request received");
                    SetState(NetworkState::Handshaking, L"Handshake started");

                    auto* req = reinterpret_cast<const HandshakeRequestPacket*>(buffer.data());
                    uint64_t newSessionId = GenerateSecureSessionID();
                    {
                        std::lock_guard<std::mutex> lock(m_mutex);
                        m_currentSession.sessionId = newSessionId;
                        
                        wchar_t wDevId[64] = {0};
                        wchar_t wDevName[32] = {0};
                        wchar_t wAppVer[16] = {0};

                        MultiByteToWideChar(CP_UTF8, 0, req->deviceId, -1, wDevId, 64);
                        MultiByteToWideChar(CP_UTF8, 0, req->deviceName, -1, wDevName, 32);
                        MultiByteToWideChar(CP_UTF8, 0, req->appVersion, -1, wAppVer, 16);

                        m_currentSession.deviceId = wDevId;
                        m_currentSession.deviceName = wDevName;
                        m_currentSession.appVersion = wAppVer;
                        m_currentSession.capabilities = req->capabilityFlags;
                        m_currentSession.clientAddr = clientAddr;
                        m_currentSession.connectedTimestamp = now;
                        m_currentSession.lastHeartbeatTimestamp = now;
                        m_currentSession.active = true;
                        m_expectedSequenceNumber = header->sequenceNumber + 1;
                    }

                    HandshakeResponsePacket resp{};
                    resp.result = HandshakeResult::Success;
                    resp.sessionId = newSessionId;
                    strcpy_s(resp.receiverVersion, "1.0.0");
                    resp.capabilityFlags = req->capabilityFlags;
                    std::copy(std::begin(req->challengeSeed), std::end(req->challengeSeed), std::begin(resp.challengeResponse));

                    SendPacket(PacketType::HandshakeResponse, reinterpret_cast<const uint8_t*>(&resp) + sizeof(TransportHeader), sizeof(HandshakeResponsePacket) - sizeof(TransportHeader));
                    SetState(NetworkState::Connected, L"Session established - Transport active");
                }
                else if (IsConnected() && header->sessionId == GetSessionID())
                {
                    m_currentSession.lastHeartbeatTimestamp = now;

                    if (header->type == PacketType::Heartbeat)
                    {
                        ProcessHeartbeat();
                    }
                    else if (header->type == PacketType::Ping)
                    {
                        PingPongPacket pong{};
                        pong.pingTimestamp = header->timestamp;
                        SendPacket(PacketType::Pong, reinterpret_cast<const uint8_t*>(&pong) + sizeof(TransportHeader), sizeof(PingPongPacket) - sizeof(TransportHeader));
                    }
                    else if (header->type == PacketType::Pong)
                    {
                        if (payloadSize >= (sizeof(PingPongPacket) - sizeof(TransportHeader)))
                        {
                            const auto* pong = reinterpret_cast<const PingPongPacket*>(buffer.data());
                            HandlePong(*pong);
                        }
                    }
                    else if (header->type == PacketType::DisconnectNotice)
                    {
                        DisconnectSession(L"Client requested disconnect");
                    }
                }
            }
        }

        if (IsConnected())
        {
            if (now - m_currentSession.lastHeartbeatTimestamp > HEARTBEAT_TIMEOUT_MS)
            {
                DisconnectSession(L"Session timeout - Heartbeat expired");
            }
            else if (now - lastPingCheckTime > PING_INTERVAL_MS)
            {
                SendPing();
                lastPingCheckTime = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void NetworkManager::SendHeartbeat()
{
    SendPacket(PacketType::Heartbeat, nullptr, 0);
}

void NetworkManager::ProcessHeartbeat()
{
    // Heartbeat received & timestamp refreshed
}

void NetworkManager::SendPing()
{
    PingPongPacket ping{};
    m_lastPingSentTime = GetCurrentTimestampMs();
    ping.pingTimestamp = m_lastPingSentTime;
    SendPacket(PacketType::Ping, reinterpret_cast<const uint8_t*>(&ping) + sizeof(TransportHeader), sizeof(PingPongPacket) - sizeof(TransportHeader));
}

void NetworkManager::HandlePong(const PingPongPacket& pong)
{
    uint64_t now = GetCurrentTimestampMs();
    uint32_t latency = static_cast<uint32_t>(now - pong.pingTimestamp);

    LatencyCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_currentSession.lastMeasuredLatencyMs = latency;
        cb = m_latencyCallback;
    }

    if (cb)
    {
        cb(latency);
    }
}

void NetworkManager::RejectConnection(HandshakeResult reason)
{
    SetState(NetworkState::Error, L"Handshake rejected (Reason code: " + std::to_wstring(static_cast<int>(reason)) + L")");
}

void NetworkManager::DisconnectSession(const std::wstring& reason)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentSession.active)
    {
        m_currentSession.active = false;
        SetState(NetworkState::Listening, L"Device disconnected: " + reason);
    }
}

uint64_t NetworkManager::GenerateSecureSessionID()
{
    uint64_t sid = 0;
    NTSTATUS status = BCryptGenRandom(nullptr, reinterpret_cast<PUCHAR>(&sid), sizeof(sid), BCRYPT_USE_SYSTEM_PREFERRED_RNG);
    if (status != 0)
    {
        sid = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    return sid;
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

bool NetworkManager::IsConnected() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state == NetworkState::Connected && m_currentSession.active;
}

uint64_t NetworkManager::GetSessionID() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSession.sessionId;
}

DeviceSession NetworkManager::GetConnectedDevice() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSession;
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

uint32_t NetworkManager::GetCurrentLatency() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSession.lastMeasuredLatencyMs;
}

std::wstring NetworkManager::GetLocalIPAddress() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_winsockInitialized) return L"127.0.0.1";

    char hostName[256] = { 0 };
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) return L"127.0.0.1";

    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(hostName, nullptr, &hints, &res) != 0 || !res) return L"127.0.0.1";

    auto* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(res->ai_addr);
    char ipStr[INET_ADDRSTRLEN] = { 0 };
    inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);
    freeaddrinfo(res);

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, NULL, 0);
    if (size_needed <= 1) return L"127.0.0.1";

    std::wstring wstr(size_needed - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, ipStr, -1, &wstr[0], size_needed);
    return wstr;
}