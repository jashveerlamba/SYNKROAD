#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <mutex>
#include "ControllerState.h"
#include "InputInjectionManager.h"

struct InputStatistics
{
    uint64_t totalPacketsReceived = 0;
    uint64_t validPacketsProcessed = 0;
    uint64_t droppedPackets = 0;
    uint64_t outOfOrderPackets = 0;
    uint32_t packetsPerSecond = 0;
};

class InputManager
{
public:
    using LogCallback = std::function<void(const std::wstring& message, bool isError)>;

    InputManager();
    ~InputManager() = default;

    bool ProcessInputPacket(const uint8_t* buffer, size_t size, uint64_t currentSessionId);
    void ResetPipeline();

    ControllerStateData GetCurrentState() const;
    InputStatistics GetInputStatistics() const;
    InputInjectionManager& GetInjectionManager() { return m_injectionManager; }

    void SetLogCallback(LogCallback callback);

private:
    bool ValidatePacketHeader(const uint8_t* buffer, size_t size, uint64_t currentSessionId);
    void UpdatePacketRate();

private:
    ControllerState m_controllerState;
    InputInjectionManager m_injectionManager;
    InputStatistics m_stats{};
    uint32_t m_lastSequenceNumber = 0;
    
    uint32_t m_packetsInCurrentSecond = 0;
    uint64_t m_lastRateCheckTimestamp = 0;

    LogCallback m_logCallback;
    mutable std::mutex m_mutex;
};