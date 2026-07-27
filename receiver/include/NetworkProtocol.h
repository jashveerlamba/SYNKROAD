#pragma once

#include <cstdint>
#include <array>
#include <vector>

#pragma pack(push, 1)

constexpr uint32_t SYNKROAD_MAGIC = 0x53594E4B; // "SYNK"
constexpr uint16_t CURRENT_PROTOCOL_VERSION = 0x0100; // v1.0
constexpr uint16_t MAX_PACKET_SIZE = 1400; // Safe MTU bound

enum class PacketType : uint8_t
{
    DiscoveryRequest  = 0x01,
    DiscoveryResponse = 0x02,
    HandshakeRequest  = 0x03,
    HandshakeResponse = 0x04,
    DisconnectNotice  = 0x05,
    Heartbeat         = 0x06,
    Ack               = 0x07,
    Ping              = 0x08,
    Pong              = 0x09,
    ControllerInput   = 0x10 // Reserved for STEP 22
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

struct TransportHeader
{
    uint32_t magic;
    uint16_t protocolVersion;
    PacketType type;
    uint64_t sessionId;
    uint32_t sequenceNumber;
    uint64_t timestamp;
    uint16_t payloadLength;
    uint32_t crc32;
};

struct DiscoveryRequestPacket
{
    TransportHeader header;
    uint64_t timestamp;
    char clientDeviceName[32];
};

struct DiscoveryResponsePacket
{
    TransportHeader header;
    uint64_t timestamp;
    char receiverName[32];
    uint16_t listeningPort;
};

struct HandshakeRequestPacket
{
    TransportHeader header;
    uint64_t timestamp;
    char deviceId[64];
    char deviceName[32];
    char appVersion[16];
    uint32_t capabilityFlags;
    uint8_t challengeSeed[16];
};

struct HandshakeResponsePacket
{
    TransportHeader header;
    uint64_t timestamp;
    HandshakeResult result;
    uint64_t sessionId;
    char receiverVersion[16];
    uint32_t capabilityFlags;
    uint8_t challengeResponse[16];
};

struct AckPacket
{
    TransportHeader header;
    uint32_t ackedSequenceNumber;
};

struct PingPongPacket
{
    TransportHeader header;
    uint64_t pingTimestamp;
};

struct DisconnectPacket
{
    TransportHeader header;
    uint8_t reasonCode;
};

#pragma pack(pop)

namespace NetworkUtils
{
    inline uint32_t CalculateCRC32(const uint8_t* data, size_t length)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i)
        {
            crc ^= data[i];
            for (int j = 0; j < 8; ++j)
            {
                crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
            }
        }
        return ~crc;
    }
}