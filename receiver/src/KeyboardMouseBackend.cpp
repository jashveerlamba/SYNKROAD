#include "KeyboardMouseBackend.h"
#include <chrono>

KeyboardMouseBackend::~KeyboardMouseBackend()
{
    Shutdown();
}

bool KeyboardMouseBackend::Initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = true;
    m_previousState = ControllerStateData{};
    m_stats = InjectionStats{};
    return true;
}

void KeyboardMouseBackend::Shutdown()
{
    ResetState();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_initialized = false;
}

void KeyboardMouseBackend::ResetState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;

    // Release default key bindings if previously held
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::A)) SendKeyboardInput(VK_SPACE, true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::B)) SendKeyboardInput(VK_CONTROL, true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::X)) SendKeyboardInput('R', true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::DPAD_UP)) SendKeyboardInput('W', true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::DPAD_DOWN)) SendKeyboardInput('S', true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::DPAD_LEFT)) SendKeyboardInput('A', true);
    if (m_previousState.buttons & static_cast<uint32_t>(ControllerButton::DPAD_RIGHT)) SendKeyboardInput('D', true);

    m_previousState = ControllerStateData{};
}

void KeyboardMouseBackend::SendKeyboardInput(WORD vkCode, bool keyUp)
{
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;

    if (SendInput(1, &input, sizeof(INPUT)) == 1)
    {
        m_stats.keyboardEvents++;
        m_stats.totalButtonEvents++;
    }
    else
    {
        m_stats.injectionFailures++;
    }
}

void KeyboardMouseBackend::SendMouseMotion(int dx, int dy)
{
    if (dx == 0 && dy == 0) return;

    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dx = static_cast<LONG>(dx);
    input.mi.dy = static_cast<LONG>(dy);
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    if (SendInput(1, &input, sizeof(INPUT)) == 1)
    {
        m_stats.mouseEvents++;
        m_stats.totalAxisUpdates++;
    }
    else
    {
        m_stats.injectionFailures++;
    }
}

void KeyboardMouseBackend::SendMouseButton(DWORD flags)
{
    INPUT input;
    ZeroMemory(&input, sizeof(INPUT));
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = flags;

    if (SendInput(1, &input, sizeof(INPUT)) == 1)
    {
        m_stats.mouseEvents++;
        m_stats.totalButtonEvents++;
    }
    else
    {
        m_stats.injectionFailures++;
    }
}

bool KeyboardMouseBackend::InjectState(const ControllerStateData& state)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    uint32_t changedButtons = state.buttons ^ m_previousState.buttons;

    // Digital Button Handling
    if (changedButtons & static_cast<uint32_t>(ControllerButton::A))
        SendKeyboardInput(VK_SPACE, !(state.buttons & static_cast<uint32_t>(ControllerButton::A)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::B))
        SendKeyboardInput(VK_CONTROL, !(state.buttons & static_cast<uint32_t>(ControllerButton::B)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::X))
        SendKeyboardInput('R', !(state.buttons & static_cast<uint32_t>(ControllerButton::X)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::DPAD_UP))
        SendKeyboardInput('W', !(state.buttons & static_cast<uint32_t>(ControllerButton::DPAD_UP)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::DPAD_DOWN))
        SendKeyboardInput('S', !(state.buttons & static_cast<uint32_t>(ControllerButton::DPAD_DOWN)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::DPAD_LEFT))
        SendKeyboardInput('A', !(state.buttons & static_cast<uint32_t>(ControllerButton::DPAD_LEFT)));

    if (changedButtons & static_cast<uint32_t>(ControllerButton::DPAD_RIGHT))
        SendKeyboardInput('D', !(state.buttons & static_cast<uint32_t>(ControllerButton::DPAD_RIGHT)));

    // Mouse Movement Injection from Analog Sticks
    int mouseX = static_cast<int>(state.rightStickX * 15.0f);
    int mouseY = static_cast<int>(-state.rightStickY * 15.0f);
    SendMouseMotion(mouseX, mouseY);

    m_previousState = state;

    auto endTime = std::chrono::high_resolution_clock::now();
    uint32_t elapsedUs = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_stats.averageLatencyUs = (m_stats.averageLatencyUs == 0) ? elapsedUs : (m_stats.averageLatencyUs + elapsedUs) / 2;

    return true;
}