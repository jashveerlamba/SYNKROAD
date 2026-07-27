#pragma once

#include <cstdint>
#include <mutex>
#include "NetworkProtocol.h"

struct ControllerStateData
{
    uint16_t profileId = 0;
    uint8_t layoutVersion = 0;
    uint32_t buttons = 0;

    int16_t steering = 0;
    uint16_t throttle = 0;
    uint16_t brake = 0;
    uint16_t clutch = 0;

    int16_t leftStickX = 0;
    int16_t leftStickY = 0;
    int16_t rightStickX = 0;
    int16_t rightStickY = 0;

    int16_t slider1 = 0;
    int16_t slider2 = 0;

    int16_t touchX = 0;
    int16_t touchY = 0;
    bool touchPressed = false;

    int16_t gyroX = 0, gyroY = 0, gyroZ = 0;
    int16_t accelX = 0, accelY = 0, accelZ = 0;

    uint64_t lastUpdateTimestamp = 0;
    uint32_t lastSequenceNumber = 0;
};

struct InputStatistics
{
    uint32_t packetsPerSecond = 0;
    uint64_t totalPacketsReceived = 0;
    uint64_t droppedPackets = 0;
    uint64_t duplicatePackets = 0;
    uint64_t invalidPackets = 0;
    double avgProcessingTimeUs = 0.0;
};

class ControllerState
{
public:
    ControllerState() = default;

    void UpdateState(const ControllerInputPayload& payload, uint32_t sequenceNumber, uint64_t timestamp);
    void Reset();

    ControllerStateData GetState() const;
    InputStatistics GetStatistics() const;

    void RecordDroppedPacket();
    void RecordDuplicatePacket();
    void RecordInvalidPacket();
    void RecordProcessingTime(double timeUs);
    void CalculatePacketsPerSecond();

private:
    mutable std::mutex m_mutex;
    ControllerStateData m_data;
    InputStatistics m_stats;

    uint32_t m_ppsCounter = 0;
    uint64_t m_lastPpsCalculationTimestamp = 0;
    double m_totalProcessingTimeAccumulator = 0.0;
    uint64_t m_processedSampleCount = 0;
};