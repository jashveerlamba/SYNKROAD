#include "InputManager.h"

InputManager::InputManager() {
    m_currentState = ControllerStateData{};
    m_stats = InputStatistics{};
}

void InputManager::ProcessInputPayload(const ControllerInputPayload& payload) {
    m_currentState.leftStickX = payload.leftStickX;
    m_currentState.leftStickY = payload.leftStickY;
    m_currentState.rightStickX = payload.rightStickX;
    m_currentState.rightStickY = payload.rightStickY;
    m_currentState.leftTrigger = payload.leftTrigger;
    m_currentState.rightTrigger = payload.rightTrigger;
    m_currentState.steeringAngle = payload.steeringAngle;
    m_currentState.accelPedal = payload.accelPedal;
    m_currentState.brakePedal = payload.brakePedal;
    m_currentState.buttons = payload.buttons;

    m_stats.totalPacketsReceived++;
}

void InputManager::UpdateState(const ControllerStateData& newState) {
    m_currentState = newState;
}