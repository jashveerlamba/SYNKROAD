#pragma once

#include <cstdint>

struct InputStatistics {
    uint64_t totalPacketsReceived = 0;
    uint64_t droppedPackets = 0;
    float averageLatencyMs = 0.0f;
};

struct ControllerStateData {
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    float steeringAngle = 0.0f;
    float accelPedal = 0.0f;
    float brakePedal = 0.0f;

    uint32_t buttons = 0;
};

class ControllerState {
public:
    ControllerState() = default;
    ~ControllerState() = default;

    void UpdateState(const ControllerStateData& state) { m_state = state; }
    const ControllerStateData& GetState() const { return m_state; }

private:
    ControllerStateData m_state{};
};