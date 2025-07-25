//
// Created by niffo on 7/16/2025.
//

#ifndef RENDERMANAGER_H
#define RENDERMANAGER_H

#include "Interface/ISystem.h"
#include "Common/DefineVulkan.h"

//~ TODO: Only for test replace with Reflection systems
#include <glm/glm.hpp>
#include <cmath>

typedef struct TEST_VERTEX_DESC
{
    glm::vec2 Position;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription()
    {
        VkVertexInputBindingDescription desc{};
        desc.binding = 0;
        desc.stride = sizeof(TEST_VERTEX_DESC);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> desc{};
        desc[0].binding = 0;
        desc[0].location = 0;
        desc[0].format = VK_FORMAT_R32G32_SFLOAT;
        desc[0].offset = offsetof(TEST_VERTEX_DESC, Position);

        desc[1].binding = 0;
        desc[1].location = 1;
        desc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        desc[1].offset = offsetof(TEST_VERTEX_DESC, Color);

        return desc;
    }

} VERTEX_DESC;


static void GenerateColorfulStarVertices(
    _fox_Out_ std::vector<VERTEX_DESC>& vertices,
    _fox_Out_ std::vector<uint16_t>&     indices,
    _fox_In_  const uint32_t            numPoints    = 5,
    _fox_In_  const float               outerRadius  = 1.0f,
    _fox_In_  const float               innerRadius  = 0.5f)
{
    vertices.clear();
    indices.clear();

    constexpr float PI = 3.14159265359f;
    constexpr float TWO_PI = 2.0f * PI;
    const float angleStep = TWO_PI / static_cast<float>(numPoints * 2);

    const glm::vec2 center = glm::vec2(0.0f);
    const glm::vec3 centerColor = glm::vec3(1.0f);

    vertices.push_back({ center, centerColor });
    const uint8_t centerIndex = 0;

    for (uint32_t i = 0; i < numPoints * 2; ++i)
    {
        const float angle = i * angleStep;
        const float radius = (i % 2 == 0) ? outerRadius : innerRadius;
        const glm::vec2 pos = glm::vec2(std::cos(angle), std::sin(angle)) * radius;

        glm::vec3 color = glm::vec3(
            0.5f + 0.5f * std::cos(angle),
            0.5f + 0.5f * std::cos(angle + 2.0f),
            0.5f + 0.5f * std::cos(angle + 4.0f)
        );

        vertices.push_back({ pos, color });

        uint8_t i1 = static_cast<uint16_t>(i + 1);
        uint8_t i2 = static_cast<uint16_t>((i + 1) % (numPoints * 2) + 1);

        indices.push_back(centerIndex);
        indices.push_back(i1);
        indices.push_back(i2);
    }
}

namespace Fox
{
    inline constexpr uint32_t MAX_FRAMES_IN_FLIGHT{ 2u };
}

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

    _fox_Return_safe VkShaderModule CreateShaderModule(_fox_In_ const std::vector<char>& code) const
    _fox_Pre_satisfies_(code.size() > 0) _fox_Success_(return != VK_NULL_HANDLE);

    void RecordCommandBuffer(
        _fox_In_ VkCommandBuffer commandBuffer,
        _fox_In_ uint32_t        imageIndex
    ) const
    _fox_Pre_satisfies_(commandBuffer != VK_NULL_HANDLE);

    //~ Helper
    void CreateBuffer(
        _fox_In_  VkDeviceSize          size,
        _fox_In_  VkBufferUsageFlags    usage,
        _fox_In_  VkMemoryPropertyFlags properties,
        _fox_Out_ VkBuffer&             buffer,
        _fox_Out_ VkDeviceMemory&       bufferMemory
    ) const;

    void CopyBufferData(
        _fox_In_ VkBuffer     src,
        _fox_In_ VkBuffer     dst,
        _fox_In_ VkDeviceSize size
    ) const;

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
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateCommandBuffers();
    void CreateSyncObjects();

    //~ Test Events
    void ReleaseSwapChain() const;
    void RecreateSwapChain();

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
    VkRenderPass     m_vkRenderPass        { VK_NULL_HANDLE };
    VkPipelineLayout m_vkPipelineLayout    { VK_NULL_HANDLE };
    VkPipeline       m_vkGraphicsPipeline  { VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeVert  { VK_NULL_HANDLE };
    VkShaderModule   m_shaderTestCubeFrag  { VK_NULL_HANDLE };
    VkCommandPool    m_vkCommandPool       { VK_NULL_HANDLE };
    VkBuffer         m_vkVertexBuffer      { VK_NULL_HANDLE };
    VkDeviceMemory   m_vkVertexBufferMemory{ VK_NULL_HANDLE };
    VkBuffer         m_vkIndexBuffer       { VK_NULL_HANDLE };
    VkDeviceMemory   m_vkIndexBufferMemory { VK_NULL_HANDLE };

    std::vector<VkCommandBuffer>  m_vkCommandBuffer;
    std::vector<VkSemaphore>      m_threadImageAvailableSemaphore;
    std::vector<VkSemaphore>      m_threadRenderFinishedSemaphore;
    std::vector<VkFence>          m_threadInFlightFences;

    std::vector<TEST_VERTEX_DESC> m_pdescVertexData{};
    std::vector<uint16_t>         m_pnIndexData{};

    uint32_t m_nVertexCounts        {   0   };
    uint32_t m_nIndexCounts         {   0   };
    uint32_t m_nCurrentFrame        {   0   };
    bool     m_bWindowResizeHandled{ false };
};

#endif //RENDERMANAGER_H
