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

    //~ System Interface Impl
    bool OnInit()         override;
    bool OnRelease()      override;

    //~ Frame Interface Impl
    void OnFrameBegin()   override;
    void OnFramePresent() override;
    void OnFrameEnd()     override;

    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;

private:
    //~ Initialize Vulkan
    bool InitVulkan();
    void CreateInstance();
    void CreateSurface();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();

    //~ Create Swap chain
    void CreateSwapChain();
    void CreateImageViews();

    //~ Test Rendering TODO: Replace it with RenderQueue
    void CreateRenderPass();
    void CreateRenderPipeline();

private:
    WindowsManager* m_pWinManager{ nullptr };

    //~ Vulkan members
    VkInstance                  m_vkInstance      { VK_NULL_HANDLE };
    VkDebugUtilsMessengerEXT    m_vkDebugMessenger{ VK_NULL_HANDLE };
    VkPhysicalDevice            m_vkPhysicalDevice{ VK_NULL_HANDLE };
    VkDevice                    m_vkDevice        { VK_NULL_HANDLE };
    VkQueue                     m_vkGraphicsQueue { VK_NULL_HANDLE };
    VkSurfaceKHR                m_vkSurface       { VK_NULL_HANDLE };
    VkQueue                     m_vkPresentQueue  { VK_NULL_HANDLE };

    //~ Swap chain members
    VkSwapchainKHR m_vkSwapChain { VK_NULL_HANDLE };
    struct
    {
        VkSurfaceFormatKHR SurfaceFormat;
        VkExtent2D         Extent;
    } m_descSwapChainSupportDetails                  {};
    std::vector<VkImage>     m_vkSwapChainImages     {};
    std::vector<VkImageView> m_vkSwapChainImageViews {};

    // TODO: Remove it later its only for test
    VkRenderPass     m_vkRenderPass      { VK_NULL_HANDLE };
    VkPipelineLayout m_vkPipelineLayout  { VK_NULL_HANDLE };
    VkPipeline       m_vkGraphicsPipeline { VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeVert{ VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeFrag{ VK_NULL_HANDLE };
};

#endif //RENDERMANAGER_H
