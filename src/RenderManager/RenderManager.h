//
// Created by niffo on 7/16/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "Interface/ISystem.h"
#include "Common/DefineVulkan.h"

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
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();

private:
    WindowsManager* m_pWinManager{ nullptr };

    //~ Vulkan members
    VkInstance                  m_vkInstance        { VK_NULL_HANDLE };
    VkDebugUtilsMessengerEXT    m_vkDebugMessenger  { VK_NULL_HANDLE };
    VkPhysicalDevice            m_vkPhysicalDevice  { VK_NULL_HANDLE };
    VkDevice                    m_vkDevice          { VK_NULL_HANDLE };
    VkQueue                     m_vkGraphicsQueue   { VK_NULL_HANDLE };
    VkSurfaceKHR                m_vkSurface         { VK_NULL_HANDLE };
    VkQueue                     m_vkPresentQueue    { VK_NULL_HANDLE };

    //~ Swap chain members
    struct
    {
        VkSurfaceFormatKHR SurfaceFormat;
        VkExtent2D Extent;
    } m_descSwapChainSupportDetails{};
    VkSwapchainKHR              m_vkSwapChain       { VK_NULL_HANDLE };
    std::vector<VkImage>        m_vkSwapChainImages {};
};

#endif //RENDERMANAGER_H
