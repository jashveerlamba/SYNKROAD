#include "IInputBackend.h"
#include "NetworkProtocol.h"
#include <windows.h>

class KeyboardMouseBackend : public IInputBackend {
public:
    void SendKeyboardInput(WORD vkCode, bool keyUp) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vkCode;
        input.ki.dwFlags = keyUp ? KEYEVENTF_KEYUP : 0;
        SendInput(1, &input, sizeof(INPUT));
    }

    void ProcessButton(ControllerButton button, bool isPressed) {
        WORD vkCode = 0;
        switch (button) {
            case ControllerButton::A:          vkCode = VK_SPACE; break;
            case ControllerButton::B:          vkCode = VK_SHIFT; break;
            case ControllerButton::X:          vkCode = 'X'; break;
            case ControllerButton::DPAD_UP:    vkCode = VK_UP; break;
            case ControllerButton::DPAD_DOWN:  vkCode = VK_DOWN; break;
            case ControllerButton::DPAD_LEFT:  vkCode = VK_LEFT; break;
            case ControllerButton::DPAD_RIGHT: vkCode = VK_RIGHT; break;
            default: return;
        }
        SendKeyboardInput(vkCode, !isPressed);
    }
};