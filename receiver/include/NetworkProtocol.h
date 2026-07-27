#pragma once

#include <cstdint>
#include <array>

#pragma pack(push, 1)

constexpr uint32_t SYNKROAD_MAGIC = 0x53594E4B; // "SYNK"
constexpr uint16_t CURRENT_PROTOCOL_VERSION = 0x0100; // v1.0

enum class PacketType : uint8_t
{
    DiscoveryRequest  = 0x01,
    DiscoveryResponse = 0x02,
    HandshakeRequest  = 0x03,
    HandshakeResponse = 0x04,
    DisconnectNotice  = 0x05,
    Heartbeat         = 0x06
};

enum class HandshakeResult : uint8_t
{
    Success              = 0x00,
    VersionMismatch      = 0x01,
    MalformedPacket      = 0x02,
    DuplicateSession     = 0x03,
    InvalidDevice        = 0x04,
    Rejected             = 0x05
};

struct PacketHeader
{
    uint32_t magic;
    uint16_t protocolVersion;
    PacketType type;
    uint16_t payloadSize;
};

struct DiscoveryRequestPacket
{
    PacketHeader header;
    uint64_t timestamp;
    char clientDeviceName[32];
};

struct DiscoveryResponsePacket
{
    PacketHeader header;
    uint64_t timestamp;
    char receiverName[32];
    uint16_t listeningPort;
};

struct HandshakeRequestPacket
{
    PacketHeader header;
    uint64_t timestamp;
    char deviceId[64];
    char deviceName[32];
    char appVersion[16];
    uint32_t capabilityFlags;
    uint8_t challengeSeed[16];
};

struct HandshakeResponsePacket
{
    PacketHeader header;
    uint64_t timestamp;
    HandshakeResult result;
    uint64_t sessionId;
    char receiverVersion[16];
    uint32_t capabilityFlags;
    uint8_t challengeResponse[16];
};

struct DisconnectPacket
{
    PacketHeader header;
    uint64_t sessionId;
    uint8_t reasonCode;
};

#pragma pack(pop)