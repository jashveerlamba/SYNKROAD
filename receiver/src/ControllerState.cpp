#include "ControllerState.h"
#include <chrono>

void ControllerState::UpdateState(const ControllerInputPayload& payload, uint32_t sequenceNumber, uint64_t timestamp)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    m_data.profileId = payload.profileId;
    m_data.layoutVersion = payload.layoutVersion;
    m_data.buttons = payload.buttons;

    m_data.steering = payload.steering;
    m_data.throttle = payload.throttle;
    m_data.brake = payload.brake;
    m_data.clutch = payload.clutch;

    m_data.leftStickX = payload.leftStickX;
    m_data.leftStickY = payload.leftStickY;
    m_data.rightStickX = payload.rightStickX;
    m_data.rightStickY = payload.rightStickY;

    m_data.slider1 = payload.slider1;
    m_data.slider2 = payload.slider2;

    m_data.touchX = payload.touchX;
    m_data.touchY = payload.touchY;
    m_data.touchPressed = (payload.touchPressed != 0);

    m_data.gyroX = payload.gyroX;
    m_data.gyroY = payload.gyroY;
    m_data.gyroZ = payload.gyroZ;
    m_data.accelX = payload.accelX;
    m_data.accelY = payload.accelY;
    m_data.accelZ = payload.accelZ;

    m_data.lastSequenceNumber = sequenceNumber;
    m_data.lastUpdateTimestamp = timestamp;

    m_stats.totalPacketsReceived++;
    m_ppsCounter++;

    CalculatePacketsPerSecond();
}

void ControllerState::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data = {};
    m_stats = {};
    m_ppsCounter = 0;
    m_lastPpsCalculationTimestamp = 0;
    m_totalProcessingTimeAccumulator = 0.0;
    m_processedSampleCount = 0;
}

ControllerStateData ControllerState::GetState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data;
}

InputStatistics ControllerState::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

void ControllerState::RecordDroppedPacket()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.droppedPackets++;
}

void ControllerState::RecordDuplicatePacket()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.duplicatePackets++;
}

void ControllerState::RecordInvalidPacket()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.invalidPackets++;
}

void ControllerState::RecordProcessingTime(double timeUs)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_totalProcessingTimeAccumulator += timeUs;
    m_processedSampleCount++;
    if (m_processedSampleCount > 0)
    {
        m_stats.avgProcessingTimeUs = m_totalProcessingTimeAccumulator / static_cast<double>(m_processedSampleCount);
    }
}

void ControllerState::CalculatePacketsPerSecond()
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (m_lastPpsCalculationTimestamp == 0)
    {
        m_lastPpsCalculationTimestamp = now;
        return;
    }

    if (now - m_lastPpsCalculationTimestamp >= 1000)
    {
        m_stats.packetsPerSecond = m_ppsCounter;
        m_ppsCounter = 0;
        m_lastPpsCalculationTimestamp = now;
    }
}