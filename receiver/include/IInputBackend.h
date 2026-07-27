#pragma once

#include <cstdint>
#include <string>
#include "ControllerState.h"

enum class BackendType
{
    KeyboardMouse,
    VirtualXbox,        // Reserved for future ViGEm Integration
    VirtualDirectInput, // Reserved for future DirectInput Integration
    CustomIntegration
};

struct InjectionStats
{
    uint64_t totalButtonEvents = 0;
    uint64_t totalAxisUpdates = 0;
    uint64_t keyboardEvents = 0;
    uint64_t mouseEvents = 0;
    uint64_t injectionFailures = 0;
    uint32_t averageLatencyUs = 0;
};

class IInputBackend
{
public:
    virtual ~IInputBackend() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool InjectState(const ControllerStateData& state) = 0;
    virtual void ResetState() = 0;

    virtual BackendType GetType() const = 0;
    virtual std::wstring GetName() const = 0;
    virtual bool IsReady() const = 0;
    virtual const InjectionStats& GetStatistics() const = 0;
};