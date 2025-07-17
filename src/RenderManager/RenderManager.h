//
// Created by niffo on 7/16/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "Interface/ISystem.h"
#include "Common/DefineVulkan.h"
#include "WindowsManager/WindowsManager.h"

class RenderManager final: public ISystem, public IFrame
{
public:
    explicit RenderManager(WindowsManager* winManager);
    ~RenderManager() override;

    bool OnInit() override;
    bool OnRelease() override;

    void OnFrameBegin() override;
    void OnFramePresent() override;
    void OnFrameEnd() override;

private:
    //~ Initialize Vulkan
    bool InitVulkan();
    void CreateInstance();

private:
    WindowsManager* m_pWinManager{ nullptr };

    //~ Vulkan members
    VkInstance m_vkInstance{ nullptr };
    VkDebugUtilsMessengerEXT m_vkDebugMessenger{ nullptr };
};

#endif //RENDERMANAGER_H
