//
// Created by niffo on 7/22/2025.
//

#ifndef FOXPLAYGROUND_H
#define FOXPLAYGROUND_H
#include <memory>

#include "DependencyResolver/DependencyResolver.h"
#include "RenderManager/RenderManager.h"
#include "WindowsManager/WindowsManager.h"
#include "Timer/Timer.h"


class FoxPlayground
{
public:
    FoxPlayground();
    ~FoxPlayground();

    bool Init();
    int Execute();

private:
    void ConfigureResources();

private:
    DependencyResolver m_resolver{};
    Timer<float>       m_timer{};

    std::unique_ptr<WindowsManager> m_pWindowsManager{ nullptr };
    std::unique_ptr<RenderManager>  m_pRenderManager { nullptr };
};

#endif //FOXPLAYGROUND_H
