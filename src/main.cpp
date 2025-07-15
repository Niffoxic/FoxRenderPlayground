#include <windows.h>
#include "WindowsManager/WindowsManager.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WindowsManager windows{};

    if (!windows.OnInit()) return EXIT_FAILURE;
    while (true)
    {
        if (const auto exitCode = WindowsManager::ProcessMessages())
        {
            return *exitCode;
        }
        Sleep(1);
    }

    return 0;
}
