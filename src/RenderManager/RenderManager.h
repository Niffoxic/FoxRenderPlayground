//
// Created by niffo on 7/16/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "Interface/ISystem.h"
#include "Common/DefineVulkan.h"
#include "Components/FxInstance.h"

class RenderManager final: public ISystem
{
    FOX_SYSTEM_GENERATOR(RenderManager);
public:
    explicit RenderManager(_fox_In_ WindowsManager* winManager);
    ~RenderManager() override;

    //~ System Interface Impl
    bool OnInit       () override _fox_Success_(return != false);
    void OnUpdateEnd  () override;
    void OnRelease    () override;

    void OnUpdateStart(float deltaTime) override;

private:
    WindowsManager*             m_pWinManager{ nullptr };
    std::unique_ptr<FxInstance> m_pInstance  { nullptr };
};

#endif //RENDERMANAGER_H
