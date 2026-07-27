#pragma once

#include <string>
#include <functional>
#include <cstdint>

#include "NetworkProtocol.h"
#include "ControllerState.h"

class InputManager
{
public:
    using LogCallback = std::function<void(const std::wstring& message, bool isError)>;

    InputManager() = default;

    bool ProcessInputPacket(const uint8_t* buffer, size_t size, uint64_t activeSessionId);
    bool ValidateInputPacket(const TransportHeader& header, const ControllerInputPayload& payload, size_t totalSize, uint64_t activeSessionId);

    ControllerStateData GetControllerState() const;
    InputStatistics GetInputStatistics() const;
    void ResetControllerState();

    void SetLogCallback(LogCallback callback);

private:
    ControllerState m_controllerState;
    uint32_t m_lastSequenceNumber = 0;
    LogCallback m_logCallback;

    static constexpr uint16_t SUPPORTED_PROFILE_ID = 0x0001;
    static constexpr uint8_t SUPPORTED_LAYOUT_VERSION = 0x01;
};