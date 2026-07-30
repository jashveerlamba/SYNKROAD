#pragma once

#include <cstdint>

enum class ControllerButton {
    A, B, X, Y,
    DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT,
    LEFT_SHOULDER, RIGHT_SHOULDER,
    START, BACK
};

struct ControllerInputPayload {
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