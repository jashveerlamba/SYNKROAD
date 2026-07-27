#pragma once

#include "IInputBackend.h"
#include <memory>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

class InputInjectionManager
{
public:
    using LogCallback = std::function<void(const std::wstring& message, bool isError)>;

    InputInjectionManager() = default;
    ~InputInjectionManager();

    bool Initialize();
    void Shutdown();

    void SetInjectionEnabled(bool enabled);
    bool IsInjectionEnabled() const;

    bool SelectBackend(BackendType type);
    IInputBackend* GetActiveBackend() const;

    bool InjectControllerState(const ControllerStateData& state);
    void ResetInjectedState();

    InjectionStats GetInjectionStatistics() const;
    void SetLogCallback(LogCallback callback);

private:
    void Log(const std::wstring& message, bool isError = false);

private:
    std::atomic<bool> m_enabled{ true };
    std::atomic<bool> m_initialized{ false };
    std::unique_ptr<IInputBackend> m_activeBackend;
    LogCallback m_logCallback;
    mutable std::mutex m_mutex;
};