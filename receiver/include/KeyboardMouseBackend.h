#pragma once

#include "IInputBackend.h"
#include <windows.h>
#include <unordered_map>
#include <mutex>

class KeyboardMouseBackend : public IInputBackend
{
public:
    KeyboardMouseBackend() = default;
    ~KeyboardMouseBackend() override;

    bool Initialize() override;
    void Shutdown() override;
    bool InjectState(const ControllerStateData& state) override;
    void ResetState() override;

    BackendType GetType() const override { return BackendType::KeyboardMouse; }
    std::wstring GetName() const override { return L"Windows SendInput (Keyboard/Mouse)"; }
    bool IsReady() const override { return m_initialized; }
    const InjectionStats& GetStatistics() const override { return m_stats; }

private:
    void SendKeyboardInput(WORD vkCode, bool keyUp);
    void SendMouseMotion(int dx, int dy);
    void SendMouseButton(DWORD flags);

private:
    bool m_initialized = false;
    ControllerStateData m_previousState{};
    InjectionStats m_stats{};
    mutable std::mutex m_mutex;
};