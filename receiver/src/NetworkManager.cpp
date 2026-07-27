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
            if (ValidatePacketHeader(req->header, PacketType::DiscoveryRequest, sizeof(DiscoveryRequestPacket)))
            {
                SetState(GetStatus(), L"Android device discovered on network");

                DiscoveryResponsePacket resp{};
                resp.header.magic = SYNKROAD_MAGIC;
                resp.header.protocolVersion = CURRENT_PROTOCOL_VERSION;
                resp.header.type = PacketType::DiscoveryResponse;
                resp.header.payloadSize = sizeof(DiscoveryResponsePacket) - sizeof(PacketHeader);
                resp.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                
                strcpy_s(resp.receiverName, "SYNKROAD Receiver");
                resp.listeningPort = m_listeningPort;

                sendto(m_discoverySocket, reinterpret_cast<const char*>(&resp), sizeof(resp), 0,
                       reinterpret_cast<sockaddr*>(&clientAddr), clientLen);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void NetworkManager::ListeningWorker()
{
    char buffer[1024];
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);

    while (m_running)
    {
        int bytesRecv = recvfrom(m_listenSocket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (bytesRecv > 0)
        {
            if (static_cast<size_t>(bytesRecv) >= sizeof(HandshakeRequestPacket))
            {
                auto* req = reinterpret_cast<HandshakeRequestPacket*>(buffer);
                if (req->header.type == PacketType::HandshakeRequest)
                {
                    SetState(NetworkState::Connecting, L"Connection request received");
                    
                    if (!ValidatePacketHeader(req->header, PacketType::HandshakeRequest, sizeof(HandshakeRequestPacket)))
                    {
                        RejectConnection(HandshakeResult::VersionMismatch);
                        continue;
                    }

                    SetState(NetworkState::Handshaking, L"Handshake started");

                    // Validation & Session Generation
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
                        m_currentSession.connectedTimestamp = req->timestamp;
                        m_currentSession.active = true;
                    }

                    HandshakeResponsePacket resp{};
                    resp.header.magic = SYNKROAD_MAGIC;
                    resp.header.protocolVersion = CURRENT_PROTOCOL_VERSION;
                    resp.header.type = PacketType::HandshakeResponse;
                    resp.header.payloadSize = sizeof(HandshakeResponsePacket) - sizeof(PacketHeader);
                    resp.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
                    resp.result = HandshakeResult::Success;
                    resp.sessionId = newSessionId;
                    strcpy_s(resp.receiverVersion, "1.0.0");
                    resp.capabilityFlags = req->capabilityFlags;

                    // Security challenge seed reflection for placeholder integrity
                    std::copy(std::begin(req->challengeSeed), std::end(req->challengeSeed), std::begin(resp.challengeResponse));

                    sendto(m_listenSocket, reinterpret_cast<const char*>(&resp), sizeof(resp), 0,
                           reinterpret_cast<sockaddr*>(&clientAddr), clientLen);

                    SetState(NetworkState::Connected, L"Handshake successful - Session established");
                }
            }
            else if (static_cast<size_t>(bytesRecv) >= sizeof(DisconnectPacket))
            {
                auto* disc = reinterpret_cast<DisconnectPacket*>(buffer);
                if (disc->header.type == PacketType::DisconnectNotice && disc->sessionId == GetSessionID())
                {
                    Disconnect();
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void NetworkManager::RejectConnection(HandshakeResult reason)
{
    SetState(NetworkState::Error, L"Handshake rejected (Reason: " + std::to_wstring(static_cast<int>(reason)) + L")");
}

void NetworkManager::Disconnect()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_currentSession.active)
    {
        m_currentSession.active = false;
        SetState(NetworkState::Listening, L"Device disconnected");
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

bool NetworkManager::ValidatePacketHeader(const PacketHeader& header, PacketType expectedType, uint16_t expectedSize)
{
    if (header.magic != SYNKROAD_MAGIC) return false;
    if (header.protocolVersion != CURRENT_PROTOCOL_VERSION) return false;
    if (header.type != expectedType) return false;
    if (header.payloadSize != (expectedSize - sizeof(PacketHeader))) return false;
    return true;
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