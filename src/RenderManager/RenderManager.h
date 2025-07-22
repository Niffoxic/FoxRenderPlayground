//
// Created by niffo on 7/16/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "Interface/ISystem.h"
#include "Common/DefineVulkan.h"

namespace Fox
{
    inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2u };
}

class RenderManager final: public ISystem, public IFrame
{
    FOX_SYSTEM_GENERATOR(RenderManager);
public:
    explicit RenderManager(_fox_In_ WindowsManager* winManager);
    ~RenderManager() override;

    //~ System Interface Impl
    bool OnInit   () override;
    bool OnRelease() override;

    //~ Frame Interface Impl
    void OnFrameBegin   () override;
    void OnFramePresent () override;
    void OnFrameEnd     () override;

    _fox_Return_safe VkShaderModule CreateShaderModule(
        _fox_In_ const std::vector<char>& code) const
    _fox_Pre_satisfies_(code.size() > 0) _fox_Success_(return != VK_NULL_HANDLE);

    void RecordCommandBuffer(
        _fox_In_ VkCommandBuffer commandBuffer,
        _fox_In_ uint32_t        imageIndex) const
    _fox_Pre_satisfies_(commandBuffer != VK_NULL_HANDLE);

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
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();

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
    } m_descSwapChainSupportDetails{};

    std::vector<VkImage>       m_vkSwapChainImages      {};
    std::vector<VkImageView>   m_vkSwapChainImageViews  {};
    std::vector<VkFramebuffer> m_vkSwapChainFramebuffers{};

    // TODO: Remove it later its only for test
    VkRenderPass     m_vkRenderPass      { VK_NULL_HANDLE };
    VkPipelineLayout m_vkPipelineLayout  { VK_NULL_HANDLE };
    VkPipeline       m_vkGraphicsPipeline{ VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeVert{ VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeFrag{ VK_NULL_HANDLE };
    VkCommandPool    m_vkCommandPool     { VK_NULL_HANDLE };

    std::vector<VkCommandBuffer>  m_vkCommandBuffer;
    std::vector<VkSemaphore>      m_threadImageAvailableSemaphore;
    std::vector<VkSemaphore>      m_threadRenderFinishedSemaphore;
    std::vector<VkFence>          m_threadInFlightFences;

    uint32_t m_nCurrentFrame{ 0 };
};

#endif //RENDERMANAGER_H
