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
    ControllerInput   = 0x10
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

// Button Bitmasks for Controller Input
namespace ControllerButtons
{
    constexpr uint32_t A            = 1 << 0;
    constexpr uint32_t B            = 1 << 1;
    constexpr uint32_t X            = 1 << 2;
    constexpr uint32_t Y            = 1 << 3;
    constexpr uint32_t LB           = 1 << 4;
    constexpr uint32_t RB           = 1 << 5;
    constexpr uint32_t L3           = 1 << 6;
    constexpr uint32_t R3           = 1 << 7;
    constexpr uint32_t START        = 1 << 8;
    constexpr uint32_t SELECT       = 1 << 9;
    constexpr uint32_t HOME         = 1 << 10;
    constexpr uint32_t DPAD_UP      = 1 << 11;
    constexpr uint32_t DPAD_DOWN    = 1 << 12;
    constexpr uint32_t DPAD_LEFT    = 1 << 13;
    constexpr uint32_t DPAD_RIGHT   = 1 << 14;
    constexpr uint32_t PADDLE_LEFT  = 1 << 15;
    constexpr uint32_t PADDLE_RIGHT = 1 << 16;
}

struct ControllerInputPayload
{
    uint16_t profileId;
    uint8_t layoutVersion;
    uint8_t flags; // Bit 0: Is Full Sync (1) vs Delta (0)

    uint32_t buttons;      // Bitmask of pressed digital buttons
    int16_t steering;      // Normalized steering (-32768 to 32767)
    uint16_t throttle;     // Trigger axis (0 to 65535)
    uint16_t brake;        // Trigger axis (0 to 65535)
    uint16_t clutch;       // Trigger/Slider axis (0 to 65535)
    
    int16_t leftStickX;    // Joysticks (-32768 to 32767)
    int16_t leftStickY;
    int16_t rightStickX;
    int16_t rightStickY;

    int16_t slider1;       // Auxiliary Sliders (-32768 to 32767)
    int16_t slider2;

    // Touchpad Placeholder
    int16_t touchX;
    int16_t touchY;
    uint8_t touchPressed;

    // Motion Sensor Placeholders (6-axis IMU)
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
};

struct ControllerInputPacket
{
    TransportHeader header;
    ControllerInputPayload inputData;
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