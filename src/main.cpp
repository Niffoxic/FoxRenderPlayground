#include "WindowsManager/WindowsManager.h"
#include "RenderManager/RenderManager.h"
#include "Engine/DependencyResolver/DependencyResolver.h"
#include "Logger/Logger.h"
#include "Timer/Timer.h"

#include <excpt.h>


LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
    if (!ExceptionInfo || !ExceptionInfo->ExceptionRecord)
        return EXCEPTION_CONTINUE_SEARCH;

    const DWORD code = ExceptionInfo->ExceptionRecord->ExceptionCode;
    const FString msg = F_TEXT("Execution Flow Error With Code: 0x") + F_TEXT(std::format("{:08X}", code));

    MessageBox(
        nullptr,
        msg.c_str(),
        F_TEXT("WinAPI Error"),
        MB_OK | MB_ICONERROR
    );

    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(
    _fox_In_ HINSTANCE hInstance,
    _fox_In_ HINSTANCE hPrevInstance,
    _fox_In_ LPSTR lpCmdLine,
    _fox_In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    SetUnhandledExceptionFilter(CrashHandler);

#if defined(_DEBUG) || defined(ENABLE_TERMINAL)
    LOGGER_INIT_DESC logDesc{};
    logDesc.FilePrefix = F_TEXT("Log_");
    logDesc.FolderPath = F_TEXT("Logs");
    logDesc.EnableTerminal = true;
    INIT_GLOBAL_LOGGER(logDesc);
#endif

    // TODO: Test Only - Replace with Facade
    try
    {
        WindowsManager  windows {};
        RenderManager   renderer{ &windows };
        Timer<float>    timer   {};

        DependencyResolver resolver{};
        resolver.Register(&windows, &renderer);
        resolver.AddDependency(&renderer, &windows);

        if (!resolver.InitializeSystems())  return EXIT_FAILURE;

        timer.Start();
        while (true)
        {
            timer.Tick();
            if (const auto exitCode = WindowsManager::ProcessMessages()) return *exitCode;

            resolver.UpdateStartSystems(timer.GetDeltaTime());

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
