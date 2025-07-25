//
// Created by niffo on 7/22/2025.
//

#include "FoxPlayground.h"

#include "WindowsManager/Inputs/KeyboardSingleton.h"
#include "WindowsManager/Inputs/MouseSingleton.h"

FoxPlayground::FoxPlayground()
{
    m_pWindowsManager = std::make_unique<WindowsManager>();
    m_pRenderManager = std::make_unique<RenderManager>(m_pWindowsManager.get());
}

FoxPlayground::~FoxPlayground()
{
    m_resolver.Clean();
}

bool FoxPlayground::Init()
{
    ConfigureResources();
    return m_resolver.InitializeSystems();
}

int FoxPlayground::Execute()
{
    m_timer.Start();
    while (true)
    {
        m_timer.Tick();
        if (const auto exitCode = WindowsManager::ProcessMessages()) return *exitCode;

        m_resolver.UpdateStartSystems(m_timer.GetDeltaTime());

#if defined(DEBUG) || defined(_DEBUG)
        KeyboardSingleton::Get().DebugKeysPressed();
        // MouseSingleton::Get().DebugKeysPressed();

        const float elapsed = m_timer.GetElapsedTime();
        m_pWindowsManager->AddOnWindowsTitle(ToFString(elapsed));
#endif

        m_resolver.UpdateEndSystems();
        Sleep(1);
    }
}

void FoxPlayground::ConfigureResources()
{
    //~ Configure Dependencies
    m_resolver.Register(m_pRenderManager.get(), m_pWindowsManager.get());
    m_resolver.AddDependency(m_pRenderManager.get(), m_pWindowsManager.get());

    m_timer.Reset();
}
