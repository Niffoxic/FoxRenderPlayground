#include "WindowsManager/WindowsManager.h"
#include "RenderManager/RenderManager.h"
#include "Logger/Logger.h"

#include "ExceptionHandler/IException.h"
#include "Timer/Timer.h"
#include <excpt.h>

LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    if (!ExceptionInfo || !ExceptionInfo->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;

    const FString msg = F_TEXT("Execution Flow Error With Code: 0x") + F_TEXT(std::format("{:08X}", code));

    MessageBoxA(
        nullptr,
        msg.c_str(),
        "WinAPI Error",
        MB_OK | MB_ICONERROR
    );

    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(
    FOX_IN HINSTANCE hInstance,
    FOX_IN HINSTANCE hPrevInstance,
    FOX_IN LPSTR lpCmdLine,
    FOX_IN int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    SetUnhandledExceptionFilter(CrashHandler);

#if defined(_DEBUG) || defined(ENABLE_TERMINAL)
    LOGGER_INIT_DESC logDesc{};
    logDesc.FilePrefix = "Log_";
    logDesc.FolderPath = "Logs";
    logDesc.EnableTerminal = true;
    INIT_GLOBAL_LOGGER(logDesc);
#endif

    try
    {
        WindowsManager  windows {};
        RenderManager   renderer{ &windows };
        Timer<float>    timer   {};

        if (!windows.OnInit())  return EXIT_FAILURE;
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
        e.SaveCrashLog(F_TEXT("CrashReport"));
        MessageBox(nullptr, e.what(), F_TEXT("Error"), MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        MessageBox(nullptr, e.what(), F_TEXT("StdException"), MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
}
