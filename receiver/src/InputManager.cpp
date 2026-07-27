#include "InputManager.h"
#include <chrono>

void InputManager::SetLogCallback(LogCallback callback)
{
    m_logCallback = std::move(callback);
}

bool InputManager::ValidateInputPacket(const TransportHeader& header, const ControllerInputPayload& payload, size_t totalSize, uint64_t activeSessionId)
{
    if (totalSize < sizeof(TransportHeader) + sizeof(ControllerInputPayload))
    {
        m_controllerState.RecordInvalidPacket();
        if (m_logCallback) m_logCallback(L"Invalid input packet: Oversized or payload truncated", true);
        return false;
    }

    if (header.sessionId != activeSessionId)
    {
        m_controllerState.RecordInvalidPacket();
        if (m_logCallback) m_logCallback(L"Invalid input packet: Session ID mismatch", true);
        return false;
    }

    if (payload.profileId != SUPPORTED_PROFILE_ID)
    {
        m_controllerState.RecordInvalidPacket();
        if (m_logCallback) m_logCallback(L"Invalid input packet: Profile ID mismatch (" + std::to_wstring(payload.profileId) + L")", true);
        return false;
    }

    if (payload.layoutVersion != SUPPORTED_LAYOUT_VERSION)
    {
        m_controllerState.RecordInvalidPacket();
        if (m_logCallback) m_logCallback(L"Invalid input packet: Unsupported layout version (" + std::to_wstring(payload.layoutVersion) + L")", true);
        return false;
    }

    return true;
}

bool InputManager::ProcessInputPacket(const uint8_t* buffer, size_t size, uint64_t activeSessionId)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    if (!buffer || size < sizeof(TransportHeader) + sizeof(ControllerInputPayload))
    {
        m_controllerState.RecordInvalidPacket();
        return false;
    }

    const auto* header = reinterpret_cast<const TransportHeader*>(buffer);
    const auto* payload = reinterpret_cast<const ControllerInputPayload*>(buffer + sizeof(TransportHeader));

    if (!ValidateInputPacket(*header, *payload, size, activeSessionId))
    {
        return false;
    }

    // Sequence & Duplicate check
    if (header->sequenceNumber <= m_lastSequenceNumber && m_lastSequenceNumber != 0)
    {
        if (header->sequenceNumber == m_lastSequenceNumber)
        {
            m_controllerState.RecordDuplicatePacket();
        }
        else
        {
            m_controllerState.RecordDroppedPacket(); // Out of order packet
        }
        return false;
    }

    m_lastSequenceNumber = header->sequenceNumber;

    m_controllerState.UpdateState(*payload, header->sequenceNumber, header->timestamp);

    auto endTime = std::chrono::high_resolution_clock::now();
    double durationUs = std::chrono::duration<double, std::micro>(endTime - startTime).count();
    m_controllerState.RecordProcessingTime(durationUs);

    return true;
}

ControllerStateData InputManager::GetControllerState() const
{
    return m_controllerState.GetState();
}

InputStatistics InputManager::GetInputStatistics() const
{
    return m_controllerState.GetStatistics();
}

void InputManager::ResetControllerState()
{
    m_lastSequenceNumber = 0;
    m_controllerState.Reset();
}