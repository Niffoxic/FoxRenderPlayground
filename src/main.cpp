#include "WindowsManager/WindowsManager.h"
#include "RenderManager/RenderManager.h"

#include <iostream>

void EnableConsole()
{
    AllocConsole();                                // Create a new console
    freopen_s(reinterpret_cast<FILE **>(stdout), "CONOUT$", "w", stdout); // Redirect stdout
    freopen_s(reinterpret_cast<FILE **>(stderr), "CONOUT$", "w", stderr); // Redirect stderr
    freopen_s(reinterpret_cast<FILE **>(stdin),  "CONIN$",  "r", stdin);  // Redirect stdin

    std::cout << "Console initialized.\n";
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

#if defined(_DEBUG)
    EnableConsole();
#endif

    //~ Tests
    WindowsManager windows{};
    RenderManager renderer{ &windows };
    try
    {
        if (!windows.OnInit()) return EXIT_FAILURE;
        if (!renderer.OnInit()) return EXIT_FAILURE;
        while (true)
        {
            if (const auto exitCode = WindowsManager::ProcessMessages())
            {
                return *exitCode;
            }
            //~ Draw
            renderer.OnFrameBegin();
            renderer.OnFramePresent();
            renderer.OnFrameEnd();

            Sleep(1);
        }
    } catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
