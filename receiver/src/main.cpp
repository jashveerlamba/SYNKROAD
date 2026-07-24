#include <windows.h>

#include "MainWindow.h"

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    MainWindow window;

    if (!window.Create(hInstance, nCmdShow))
    {
        MessageBoxW(
            nullptr,
            L"Failed to create SYNKROAD Receiver window.",
            L"SYNKROAD",
            MB_ICONERROR | MB_OK);

        return -1;
    }

    return window.Run();
}