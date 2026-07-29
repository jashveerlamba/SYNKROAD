#pragma once

#include "ControllerState.h"
#include "NetworkProtocol.h"

class InputManager {
public:
    InputManager();
    ~InputManager() = default;

    void ProcessInputPayload(const ControllerInputPayload& payload);
    void UpdateState(const ControllerStateData& newState);
    
    const ControllerStateData& GetState() const { return m_currentState; }
    const InputStatistics& GetStatistics() const { return m_stats; }

private:
    ControllerStateData m_currentState{};
    InputStatistics m_stats{};
};