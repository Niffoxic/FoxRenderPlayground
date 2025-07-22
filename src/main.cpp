#include "WindowsManager/WindowsManager.h"
#include "RenderManager/RenderManager.h"
#include "Logger/Logger.h"

#include "ExceptionHandler/IException.h"

#include <iostream>

#include "Timer/Timer.h"

int WINAPI WinMain(
    FOX_IN HINSTANCE hInstance,
    FOX_IN HINSTANCE hPrevInstance,
    FOX_IN LPSTR lpCmdLine,
    FOX_IN int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);

#if defined(_DEBUG) || defined(ENABLE_TERMINAL)
    LOGGER_INIT_DESC logDesc{};
    logDesc.FilePrefix = "Log_";
    logDesc.FolderPath = "Logs";
    logDesc.EnableTerminal = true;
    INIT_GLOBAL_LOGGER(logDesc);
#endif

    try
    {
        WindowsManager windows{};
        RenderManager renderer{ &windows };
        Timer<float> timer{};

        if (!windows.OnInit()) return EXIT_FAILURE;
        if (!renderer.OnInit()) return EXIT_FAILURE;

        timer.Start();
        while (true)
        {
            if (const auto exitCode = WindowsManager::ProcessMessages())
                return *exitCode;
            timer.Tick();

            renderer.OnFrameBegin();
            renderer.OnFramePresent();
            renderer.OnFrameEnd();

            const float elapsed = timer.GetElapsedTime();
            windows.AddOnWindowsTitle(ToFString(elapsed));
            Sleep(1);
        }
    }
    catch (const IException& e)
    {
        e.SaveCrashLog("CrashReport");
        MessageBox(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        MessageBoxA(nullptr, e.what(), "StdException", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
}
