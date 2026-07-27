#include "InputInjectionManager.h"
#include "KeyboardMouseBackend.h"

InputInjectionManager::~InputInjectionManager()
{
    Shutdown();
}

void InputInjectionManager::SetLogCallback(LogCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logCallback = std::move(callback);
}

void InputInjectionManager::Log(const std::wstring& message, bool isError)
{
    LogCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        cb = m_logCallback;
    }
    if (cb) cb(message, isError);
}

bool InputInjectionManager::Initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_initialized) return true;

    m_activeBackend = std::make_unique<KeyboardMouseBackend>();
    if (!m_activeBackend->Initialize())
    {
        Log(L"Input Injection Framework: Failed to initialize primary backend", true);
        return false;
    }

    m_initialized = true;
    Log(L"Input Injection Framework Initialized. Active Backend: " + m_activeBackend->GetName());
    return true;
}

void InputInjectionManager::Shutdown()
{
    ResetInjectedState();

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeBackend)
    {
        m_activeBackend->Shutdown();
        m_activeBackend.reset();
    }
    m_initialized = false;
    Log(L"Input Injection Framework shut down gracefully.");
}

void InputInjectionManager::SetInjectionEnabled(bool enabled)
{
    if (m_enabled != enabled)
    {
        m_enabled = enabled;
        if (!m_enabled) ResetInjectedState();
        Log(m_enabled ? L"Input Injection ENABLED" : L"Input Injection DISABLED");
    }
}

bool InputInjectionManager::IsInjectionEnabled() const
{
    return m_enabled && m_initialized;
}

bool InputInjectionManager::SelectBackend(BackendType type)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ResetInjectedState();

    if (m_activeBackend) m_activeBackend->Shutdown();

    switch (type)
    {
    case BackendType::KeyboardMouse:
        m_activeBackend = std::make_unique<KeyboardMouseBackend>();
        break;
    default:
        Log(L"Selected backend type is unsupported or not implemented", true);
        return false;
    }

    bool ok = m_activeBackend->Initialize();
    if (ok) Log(L"Switched active injection backend to: " + m_activeBackend->GetName());
    return ok;
}

IInputBackend* InputInjectionManager::GetActiveBackend() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_activeBackend.get();
}

bool InputInjectionManager::InjectControllerState(const ControllerStateData& state)
{
    if (!IsInjectionEnabled()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_activeBackend || !m_activeBackend->IsReady()) return false;

    return m_activeBackend->InjectState(state);
}

void InputInjectionManager::ResetInjectedState()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeBackend && m_activeBackend->IsReady())
    {
        m_activeBackend->ResetState();
        Log(L"Input Injection: Released all active key and button states");
    }
}

InjectionStats InputInjectionManager::GetInjectionStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeBackend)
    {
        return m_activeBackend->GetStatistics();
    }
    return {};
}