#include "InputManager.h"
#include "NetworkProtocol.h"
#include <chrono>

InputManager::InputManager()
{
    m_injectionManager.Initialize();
}

void InputManager::SetLogCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logCallback = callback;
    m_injectionManager.SetLogCallback(callback);
}

void InputManager::ResetPipeline()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_controllerState.Reset();
    m_injectionManager.ResetInjectedState();
    m_stats = {};
    m_lastSequenceNumber = 0;
}

bool InputManager::ValidatePacketHeader(const uint8_t* buffer, size_t size, uint64_t currentSessionId)
{
    if (!buffer || size < sizeof(TransportHeader) + sizeof(ControllerInputPayload)) return false;

    auto* header = reinterpret_cast<const TransportHeader*>(buffer);
    if (header->magic != SYNKROAD_MAGIC) return false;
    if (header->type != PacketType::ControllerInput) return false;
    if (header->sessionId != currentSessionId) return false;

    return true;
}

void InputManager::UpdatePacketRate()
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    m_packetsInCurrentSecond++;
    if (now - m_lastRateCheckTimestamp >= 1000)
    {
        m_stats.packetsPerSecond = m_packetsInCurrentSecond;
        m_packetsInCurrentSecond = 0;
        m_lastRateCheckTimestamp = now;
    }
}

bool InputManager::ProcessInputPacket(const uint8_t* buffer, size_t size, uint64_t currentSessionId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stats.totalPacketsReceived++;

    if (!ValidatePacketHeader(buffer, size, currentSessionId))
    {
        m_stats.droppedPackets++;
        return false;
    }

    auto* header = reinterpret_cast<const TransportHeader*>(buffer);
    if (m_lastSequenceNumber != 0 && header->sequenceNumber <= m_lastSequenceNumber)
    {
        m_stats.outOfOrderPackets++;
        m_stats.droppedPackets++;
        return false;
    }

    m_lastSequenceNumber = header->sequenceNumber;

    auto* payload = reinterpret_cast<const ControllerInputPayload*>(buffer + sizeof(TransportHeader));
    
    ControllerStateData newState{};
    newState.buttons = payload->buttons;
    newState.leftTrigger = payload->leftTrigger;
    newState.rightTrigger = payload->rightTrigger;
    newState.leftStickX = payload->leftStickX;
    newState.leftStickY = payload->leftStickY;
    newState.rightStickX = payload->rightStickX;
    newState.rightStickY = payload->rightStickY;
    newState.steeringAngle = payload->steeringAngle;
    newState.accelPedal = payload->accelPedal;
    newState.brakePedal = payload->brakePedal;

    m_controllerState.UpdateState(newState);
    m_stats.validPacketsProcessed++;
    UpdatePacketRate();

    // Dispatch to Windows Input Injection Manager
    m_injectionManager.InjectControllerState(m_controllerState.GetState());

    return true;
}

ControllerStateData InputManager::GetCurrentState() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_controllerState.GetState();
}

InputStatistics InputManager::GetInputStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}