//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"
#include "Helper.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

RenderManager::RenderManager(WindowsManager *winManager)
 : m_pWinManager(winManager)
{}

RenderManager::~RenderManager()
{
    OnRelease();
}

bool RenderManager::OnInit()
{
    return InitVulkan();
}

void RenderManager::OnUpdateStart(const float deltaTime)
{
    m_nElapsedTime += deltaTime;
    vkDeviceWaitIdle(m_vkDevice);

    if (m_threadImageAvailableSemaphore.empty()
        || m_threadInFlightFences.empty()
        || m_threadRenderFinishedSemaphore.empty()) return;

    if (m_nCurrentFrame >= Fox::MAX_FRAMES_IN_FLIGHT)
        THROW_EXCEPTION_FMT("Current Count Exceeded: {}/{}", m_nCurrentFrame, Fox::MAX_FRAMES_IN_FLIGHT);

    UpdateUniformBuffer(m_nCurrentFrame);
    vkWaitForFences(m_vkDevice, 1, &m_threadInFlightFences[m_nCurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        m_vkDevice,
        m_vkSwapChain,
        UINT64_MAX,
        m_threadImageAvailableSemaphore[m_nCurrentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        RecreateSwapChain();
        return;
    }
    if (result != VK_SUCCESS & result != VK_SUBOPTIMAL_KHR) THROW_EXCEPTION_MSG("Failed to get swap chain image");

    vkResetFences(m_vkDevice, 1, &m_threadInFlightFences[m_nCurrentFrame]);
    vkResetCommandBuffer(m_vkCommandBuffer[m_nCurrentFrame], 0);

    RecordCommandBuffer(m_vkCommandBuffer[m_nCurrentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[]{ m_threadImageAvailableSemaphore[m_nCurrentFrame] };
    VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vkCommandBuffer[m_nCurrentFrame];

    VkSemaphore signalSemaphores[]{ m_threadRenderFinishedSemaphore[m_nCurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, m_threadInFlightFences[m_nCurrentFrame]) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to Draw CB");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_vkSwapChain };
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &imageIndex;

    result = vkQueuePresentKHR(m_vkPresentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_bWindowResizeHandled) RecreateSwapChain();
    else if (result != VK_SUCCESS) THROW_EXCEPTION_MSG("Failed to present");

    m_nCurrentFrame = (m_nCurrentFrame + 1) % Fox::MAX_FRAMES_IN_FLIGHT;
}

void RenderManager::OnUpdateEnd()
{

}

void RenderManager::OnRelease()
{
    vkDeviceWaitIdle(m_vkDevice);

    vkDestroyBuffer(m_vkDevice, m_vkVertexBuffer, nullptr);
    vkFreeMemory(m_vkDevice, m_vkVertexBufferMemory, nullptr);

    vkDestroyBuffer(m_vkDevice, m_vkIndexBuffer, nullptr);
    vkFreeMemory(m_vkDevice, m_vkIndexBufferMemory, nullptr);

    //~ Clean Uniform buffers
    for (uint32_t i = 0; i < Fox::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(m_vkDevice, m_pvkUniformBuffers[i], nullptr);
        vkFreeMemory(m_vkDevice, m_pvkUniformBuffersMemory[i], nullptr);
    }

    //~ Release thread locks
    for (size_t i = 0; i < Fox::MAX_FRAMES_IN_FLIGHT; i++)
    {
		vkDestroySemaphore(m_vkDevice, m_threadImageAvailableSemaphore[i], nullptr);
		vkDestroySemaphore(m_vkDevice, m_threadRenderFinishedSemaphore[i], nullptr);
		vkDestroyFence(m_vkDevice, m_threadInFlightFences[i], nullptr);
    }

    //~ Clear Render Related stuff
    vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

    vkDestroyShaderModule(m_vkDevice, m_shaderTestCubeFrag, nullptr);
    vkDestroyShaderModule(m_vkDevice, m_shaderTestCubeVert, nullptr);

    vkDestroyPipeline(m_vkDevice, m_vkGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_vkDevice, m_vkPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_vkDevice, m_vkDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_vkDevice, m_vkDescriptorPool, nullptr);
    vkDestroyRenderPass(m_vkDevice, m_vkRenderPass, nullptr);

    //~ Clean Swap chain
    ReleaseSwapChain();

    //~ Clean Vulkan resources
    vkDestroyDevice(m_vkDevice, nullptr);

#if defined(DEBUG) || defined(_DEBUG)
    Fox::DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
#endif

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);
}

VkShaderModule RenderManager::CreateShaderModule(const std::vector<char>& code) const
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;

    if (vkCreateShaderModule(m_vkDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create shader module!");

    return shaderModule;
}

bool RenderManager::InitVulkan()
{
    //~ Init Vulkan
    CreateInstance();
    CreateSurface();

#if defined(DEBUG) || defined(_DEBUG)
    Fox::CreateDebugUtilsMessenger(m_vkInstance, m_vkDebugMessenger);
#endif

    SelectPhysicalDevice();
    CreateLogicalDevice();

    //~ Init Swap chain
    CreateSwapChain();
    CreateImageViews();

    //~ TODO: Only For Test replace with Spatial structure
    CreateRenderPass();
    CreateDescriptorSetLayout();
    CreateRenderPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateTextureImage();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffer();
    CreateDescriptorPool();
    CreateDescriptorSets();
    CreateCommandBuffers();
    CreateSyncObjects();
    return true;
}

void RenderManager::CreateInstance()
{
    LOG_WARNING("Trying to create a new VkInstance instance");
    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = m_pWinManager->GetWindowsTitle().c_str();
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0); // TODO: Get it directly from CMake can ya!!
    appInfo.pEngineName         = "RenderEngine"; // TODO: This as well
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0); // TODO: umm hmm
    appInfo.apiVersion          = VK_API_VERSION_1_4; // WE DO NOT CARE ABOUT ANYTHING LESS

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    constexpr auto requiredExtensions = Fox::vkRequiredInstanceExtensions;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

// Add Debug Extension
#if defined(DEBUG) || defined(_DEBUG)
    Fox::PrintAvailableInstanceExtensions();
    if (!Fox::CheckValidationLayerSupport()) THROW_EXCEPTION();

    createInfo.enabledLayerCount   = static_cast<uint32_t>(Fox::vkValidationLayers.size());
    createInfo.ppEnabledLayerNames = Fox::vkValidationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    Fox::PopulateDebugMessengerCreateInfo(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)(&debugCreateInfo);

#else
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
#endif

    if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create instance.");

    LOG_SUCCESS("Vulkan Instance Created!");
}

void RenderManager::CreateSurface()
{
    LOG_WARNING("Trying to create window surface");
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd      = m_pWinManager->GetWinHandle();
    surfaceCreateInfo.hinstance = m_pWinManager->GetWinHInstance();

    if (vkCreateWin32SurfaceKHR(m_vkInstance, &surfaceCreateInfo, nullptr, &m_vkSurface) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create window surface!");

    LOG_SUCCESS("Vulkan surface created!");
}

void RenderManager::SelectPhysicalDevice()
{
    LOG_WARNING("Looking for physical Device");
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, nullptr);
    if (deviceCount == 0) THROW_EXCEPTION_MSG("No Physics Video Device Found!");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vkInstance, &deviceCount, devices.data());

    //~ Selecting whatever is suitable I DONT GIVE A SHEEET
    for (const auto& device : devices)
    {
        if (Fox::IsDeviceSuitable(device, m_vkSurface))
        {
            m_vkPhysicalDevice = device;
            break;
        }
    }

    if (m_vkPhysicalDevice == VK_NULL_HANDLE) THROW_EXCEPTION_MSG("Failed to find a suitable GPU!");
    LOG_SUCCESS("Found a suitable GPU!");
}

void RenderManager::CreateLogicalDevice()
{
    LOG_WARNING("Trying to Create Logical Device");
    const QUEUE_FAMILY_INDEX_DESC desc = Fox::FindQueueFamily(m_vkPhysicalDevice, m_vkSurface);
    if (not desc.IsInitialized()) THROW_EXCEPTION_MSG("Not a suitable family desc");

    std::set uniqueQueueFamilies{ desc.GraphicsFamily.value(), desc.PresentFamily.value() };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    constexpr float queuePriority = 1.0f;

    for (uint32_t queueFamilyIndex : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.emplace_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType                    = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount     = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos        = queueCreateInfos.data();
    createInfo.pEnabledFeatures         = &deviceFeatures;
    createInfo.enabledExtensionCount    = static_cast<uint32_t>(Fox::vkDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames  = Fox::vkDeviceExtensions.data();
    createInfo.enabledLayerCount        = 0;

#if defined(DEBUG) || defined(_DEBUG)
    createInfo.enabledLayerCount = static_cast<uint32_t>(Fox::vkValidationLayers.size());
    createInfo.ppEnabledLayerNames = Fox::vkValidationLayers.data();
#endif

    if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create logical device!");

    vkGetDeviceQueue(m_vkDevice, desc.GraphicsFamily.value(), 0, &m_vkGraphicsQueue);
    vkGetDeviceQueue(m_vkDevice, desc.PresentFamily.value(), 0, &m_vkPresentQueue);
    LOG_SUCCESS("Vulkan Device Created!");
}

void RenderManager::CreateSwapChain()
{
    LOG_WARNING("Attempting to create swap chain");
    const SWAP_CHAIN_SUPPORT_DESC desc     = Fox::QuerySwapChainSupport(m_vkPhysicalDevice, m_vkSurface);
    const VkSurfaceFormatKHR surfaceFormat = Fox::SelectSwapChainFormat(desc.Formats);
    const VkPresentModeKHR presentMode     = Fox::SelectSwapChainPresentMode(desc.PresentModes);
    const VkExtent2D extent                = Fox::SelectSwapChainExtent(desc.Capabilities, m_pWinManager);

    uint32_t imageCount = desc.Capabilities.minImageCount + 1;

    if (desc.Capabilities.maxImageCount > 0 && imageCount > desc.Capabilities.maxImageCount)
        imageCount = desc.Capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = m_vkSurface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = extent;
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageArrayLayers = 1;
    createInfo.presentMode      = presentMode;

    const QUEUE_FAMILY_INDEX_DESC qDesc = Fox::FindQueueFamily(m_vkPhysicalDevice, m_vkSurface);
    const uint32_t qFamilyIndices[]{ qDesc.GraphicsFamily.value(), qDesc.PresentFamily.value() };

    if (qDesc.GraphicsFamily.value() != qDesc.PresentFamily.value())
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = qFamilyIndices;
    }else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
    }

    createInfo.preTransform   = desc.Capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_vkDevice, &createInfo, nullptr, &m_vkSwapChain) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create swap chain!");

    //~ Caching for future use
    m_descSwapChainSupportDetails.SurfaceFormat = surfaceFormat;
    m_descSwapChainSupportDetails.Extent = extent;

    LOG_SUCCESS("Created swap chain!");

    //~ Retrieve bb image or render output whateverr.....
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, nullptr);
    m_vkSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_vkDevice, m_vkSwapChain, &imageCount, m_vkSwapChainImages.data());
}

void RenderManager::CreateImageViews()
{
    LOG_WARNING("Attempting to create image views counts: {}", m_vkSwapChainImages.size());
    m_vkSwapChainImageViews.resize(m_vkSwapChainImages.size());

    for (size_t i = 0; i < m_vkSwapChainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image    = m_vkSwapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format   = m_descSwapChainSupportDetails.SurfaceFormat.format;

        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel   = 0;
        viewInfo.subresourceRange.levelCount     = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount     = 1;

        if (vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &m_vkSwapChainImageViews[i]) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed to create image views!");

        LOG_INFO("[{}] added image views", i);
    }

    LOG_SUCCESS("Created all image views!");
}

void RenderManager::CreateRenderPass()
{
    LOG_INFO("Attempting to create render pass...");
    VkAttachmentDescription colorAttachments{};
    colorAttachments.format         = m_descSwapChainSupportDetails.SurfaceFormat.format;
    colorAttachments.samples        = VK_SAMPLE_COUNT_1_BIT;
    colorAttachments.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachments.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachments.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachments.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    colorAttachments.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachments.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &colorAttachmentReference;

    VkSubpassDependency subpassDependency{};
    subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependency.dstSubpass = 0;
    subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.srcAccessMask = 0;
    subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType            = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount  = 1;
    renderPassInfo.subpassCount     = 1;
    renderPassInfo.pAttachments     = &colorAttachments;
    renderPassInfo.pSubpasses       = &subpass;
    renderPassInfo.dependencyCount  = 1;
    renderPassInfo.pDependencies    = &subpassDependency;

    if (vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_vkRenderPass) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create render pass!");

    LOG_INFO("Created render pass!");
}

void RenderManager::CreateDescriptorSetLayout()
{
    LOG_WARNING("Attempting to create descriptor set layout...");
    VkDescriptorSetLayoutBinding desc{};
    desc.binding = 0;
    desc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    desc.descriptorCount = 1;
    desc.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    desc.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &desc;

    if (vkCreateDescriptorSetLayout(m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create descriptor set layout!");

    LOG_SUCCESS("Created descriptor set layout!");
}

void RenderManager::CreateRenderPipeline()
{
    LOG_WARNING("Attempting to Create Render Pipeline");
    //~ Only for test
    const auto vertCode = FileSystem::ReadFromFile("compiled_shaders/test_triangle/mul-triangle-vert.spv");
    const auto fragCode = FileSystem::ReadFromFile("compiled_shaders/test_triangle/mul-triangle-frag.spv");

    m_shaderTestCubeVert = CreateShaderModule(vertCode);
    m_shaderTestCubeFrag = CreateShaderModule(fragCode);

    //~ Config Vertex Shader
    VkPipelineShaderStageCreateInfo vertStageInfo{};
    vertStageInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStageInfo.stage     = VK_SHADER_STAGE_VERTEX_BIT;
    vertStageInfo.module    = m_shaderTestCubeVert;
    vertStageInfo.pName     = "main";

    //~ Config Frag Shader
    VkPipelineShaderStageCreateInfo fragStageInfo{};
    fragStageInfo.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStageInfo.stage     = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStageInfo.module    = m_shaderTestCubeFrag;
    fragStageInfo.pName     = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

    //~ IA
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //~ Vertex Data
    auto vertexDesc = VERTEX_DESC::GetBindingDescription();
    auto attributeDesc = VERTEX_DESC::GetAttributeDescriptions();
    vertexInputInfo.vertexBindingDescriptionCount       = 1;
    vertexInputInfo.vertexAttributeDescriptionCount     = static_cast<uint32_t>(attributeDesc.size());
    vertexInputInfo.pVertexAttributeDescriptions        = attributeDesc.data();
    vertexInputInfo.pVertexBindingDescriptions          = &vertexDesc;

    //~ Not indices so we gonna send it
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //~ Viewport
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount  = 1;

    //~ Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizationState{};
    rasterizationState.sType                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable         = VK_FALSE;
    rasterizationState.rasterizerDiscardEnable  = VK_FALSE;
    rasterizationState.polygonMode              = VK_POLYGON_MODE_FILL;
    rasterizationState.lineWidth                = 1.0f;
    rasterizationState.cullMode                 = VK_CULL_MODE_BACK_BIT;
    rasterizationState.frontFace                = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable          = VK_FALSE;

    //~ MSAA
    VkPipelineMultisampleStateCreateInfo msaa{};
    msaa.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msaa.sampleShadingEnable    = VK_FALSE;
    msaa.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT;

    //~ bs
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
          VK_COLOR_COMPONENT_R_BIT
        | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT
        | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    //~ Blend State
    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable       = VK_FALSE;
    colorBlendState.logicOp             = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount     = 1;
    colorBlendState.pAttachments        = &colorBlendAttachment;
    colorBlendState.blendConstants[0]   = 0.0f;
    colorBlendState.blendConstants[1]   = 0.0f;
    colorBlendState.blendConstants[2]   = 0.0f;
    colorBlendState.blendConstants[3]   = 0.0f;

    std::vector<VkDynamicState> dynamicStates
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount  = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates     = dynamicStates.data();

    //~ Pipeline
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount         = 1;
    pipelineLayoutInfo.pSetLayouts            = &m_vkDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    if (vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_vkPipelineLayout) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed To Create Test pipeline Layout");

    LOG_SUCCESS("Pipeline Layout Created!");
    LOG_WARNING("Attempting to create Graphics Pipeline");

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount             = ARRAYSIZE(shaderStages);
    pipelineInfo.pStages                = shaderStages;
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizationState;
    pipelineInfo.pMultisampleState      = &msaa;
    pipelineInfo.pColorBlendState       = &colorBlendState;
    pipelineInfo.pDynamicState          = &dynamicState;
    pipelineInfo.pDepthStencilState     = nullptr;
    pipelineInfo.layout                 = m_vkPipelineLayout;
    pipelineInfo.renderPass             = m_vkRenderPass;
    pipelineInfo.subpass                = 0;
    pipelineInfo.basePipelineHandle     = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex      = -1;

    if (vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE,
        1, &pipelineInfo, nullptr, &m_vkGraphicsPipeline) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed creating graphics pipeline");

    LOG_SUCCESS("Graphics Pipeline Created");
}

void RenderManager::CreateFramebuffers()
{
    LOG_WARNING("Attempting to create frame buffers");
    m_vkSwapChainFramebuffers.resize(m_vkSwapChainImageViews.size());

    for (size_t i = 0; i < m_vkSwapChainImageViews.size(); i++)
    {
        const VkImageView attachments[]{ m_vkSwapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = m_vkRenderPass;
        framebufferInfo.attachmentCount = ARRAYSIZE(attachments);
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.height          = m_descSwapChainSupportDetails.Extent.height;
        framebufferInfo.width           = m_descSwapChainSupportDetails.Extent.width;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(m_vkDevice,
            &framebufferInfo, nullptr, &m_vkSwapChainFramebuffers[i]) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed creating framebuffer");
    }
    LOG_SUCCESS("Framebuffer Created Counts: {}", m_vkSwapChainFramebuffers.size());
}

void RenderManager::CreateCommandPool()
{
    LOG_WARNING("Attempting to create command pool");
    //~ Command Pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex   = Fox::FindQueueFamily(m_vkPhysicalDevice, m_vkSurface).GraphicsFamily.value();
    poolInfo.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed creating command pool");

    LOG_SUCCESS("Command pool Created");
}

void RenderManager::CreateTextureImage()
{
    LOG_WARNING("Attempting to create vulkan texture image buffer");
    int textureWidth, textureHeight, textureChannels;

    const stbi_uc* pixels = stbi_load(
        "",
        &textureWidth,
        &textureHeight,
        &textureChannels,
        STBI_rgb_alpha
    );

    const VkDeviceSize imageSize = textureWidth * textureHeight * 4u;
    if (not pixels) THROW_EXCEPTION_MSG("Failed creating texture image");
    LOG_SUCCESS("Texture Image Created");

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    CreateBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(m_vkDevice, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, imageSize);
    vkUnmapMemory(m_vkDevice, stagingBufferMemory);

    stbi_image_free(&pixels);

    CreateImage(
        textureWidth,
        textureHeight,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vkTextureImage,
        m_vkTextureImageMemory
    );
    LOG_SUCCESS("Texture vulkan Image buffer created");
}

void RenderManager::CreateVertexBuffer()
{
    LOG_WARNING("Attempting to create main vertex buffer");
    GenerateColorfulStarVertices(m_pdescVertexData, m_pnIndexData);
    m_nVertexCounts = static_cast<uint32_t>(m_pdescVertexData.size());
    m_nIndexCounts = static_cast<uint32_t>(m_pnIndexData.size());
    const VkDeviceSize bufferSize = sizeof(m_pdescVertexData[0]) * m_nVertexCounts;

    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    CreateBuffer(
         bufferSize,
         VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         stagingBuffer,
         stagingBufferMemory
     );

    void* data;
    vkMapMemory(m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, &m_pdescVertexData[0], bufferSize);
    vkUnmapMemory(m_vkDevice, stagingBufferMemory);

    CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vkVertexBuffer,
        m_vkVertexBufferMemory
    );

    CopyBufferData(stagingBuffer, m_vkVertexBuffer, bufferSize);

    vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_vkDevice, stagingBufferMemory, nullptr);

    LOG_SUCCESS("Vertex Buffer Created");
}

void RenderManager::CreateIndexBuffer()
{
    VkDeviceSize  bufferSize = sizeof(m_pnIndexData[0]) * m_nIndexCounts;

    VkBuffer stagingBuffer{};
    VkDeviceMemory stagingBufferMemory{};

    CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(m_vkDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, m_pnIndexData.data(), bufferSize);
    vkUnmapMemory(m_vkDevice, stagingBufferMemory);

    CreateBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_vkIndexBuffer,
        m_vkIndexBufferMemory
    );

    CopyBufferData(stagingBuffer, m_vkIndexBuffer, bufferSize);

    vkDestroyBuffer(m_vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(m_vkDevice, stagingBufferMemory, nullptr);
}

void RenderManager::CreateUniformBuffer()
{
    LOG_INFO("Attempting to create uniform buffer");
    m_pvkUniformBuffers      .resize(Fox::MAX_FRAMES_IN_FLIGHT);
    m_ppUniformBuffersMapped .resize(Fox::MAX_FRAMES_IN_FLIGHT);
    m_pvkUniformBuffersMemory.resize(Fox::MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < Fox::MAX_FRAMES_IN_FLIGHT; i++)
    {
        constexpr VkDeviceSize bufferSize = sizeof(VERTEX_UNIFORM_DATA_DESC);
        CreateBuffer(
            bufferSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            m_pvkUniformBuffers[i],
            m_pvkUniformBuffersMemory[i]
        );

        vkMapMemory(m_vkDevice, m_pvkUniformBuffersMemory[i], 0, bufferSize, 0, &m_ppUniformBuffersMapped[i]);
    }
    LOG_SUCCESS("Uniform Buffer Created");
}

void RenderManager::CreateDescriptorPool()
{
    LOG_WARNING("Attempting to create descriptor pool");
    VkDescriptorPoolSize descriptorPoolSize{};
    descriptorPoolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorPoolSize.descriptorCount = Fox::MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &descriptorPoolSize;
    poolInfo.maxSets       = Fox::MAX_FRAMES_IN_FLIGHT;

    if (vkCreateDescriptorPool(m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create descriptor pool");

    LOG_SUCCESS("Descriptor pool created");
}

void RenderManager::CreateDescriptorSets()
{
    LOG_WARNING("Attempting to create descriptor sets");
    std::vector<VkDescriptorSetLayout> layouts(Fox::MAX_FRAMES_IN_FLIGHT, m_vkDescriptorSetLayout);

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
    descriptorSetAllocateInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.descriptorPool     = m_vkDescriptorPool;
    descriptorSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(Fox::MAX_FRAMES_IN_FLIGHT);
    descriptorSetAllocateInfo.pSetLayouts        = layouts.data();

    m_pvkDescSets.resize(Fox::MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(m_vkDevice, &descriptorSetAllocateInfo, m_pvkDescSets.data()) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create Descriptor Sets");

    LOG_SUCCESS("Descriptor Sets created");

    for (uint32_t i = 0; i < Fox::MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_pvkUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(VERTEX_UNIFORM_DATA_DESC);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = m_pvkDescSets[i];
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        descriptorWrite.pImageInfo       = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(m_vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
}

void RenderManager::CreateCommandBuffers()
{
    m_vkCommandBuffer.resize(Fox::MAX_FRAMES_IN_FLIGHT);
    LOG_WARNING("Attempting to create command buffers");
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_vkCommandPool;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_vkCommandBuffer.size());

    if (vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffer.data()) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed creating command buffers");

    LOG_SUCCESS("Command buffer Created");
}

void RenderManager::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType             = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags             = 0;
    beginInfo.pInheritanceInfo  = nullptr;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed recording command buffer");

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType                = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass           = m_vkRenderPass;
    renderPassInfo.framebuffer          = m_vkSwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset    = { 0, 0 };
    renderPassInfo.renderArea.extent    = m_descSwapChainSupportDetails.Extent;

    const auto color = GenerateRandomColor();
    VkClearValue clearColor{};
    clearColor.color                = { color[0], color[1], color[2], color[3] };
    renderPassInfo.clearValueCount  = 1;
    renderPassInfo.pClearValues     = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkGraphicsPipeline);

    //~ Viewport
    VkViewport viewport{};
    viewport.x          = 0.0f;
    viewport.y          = 0.0f;
    viewport.width      = static_cast<float>(m_descSwapChainSupportDetails.Extent.width);
    viewport.height     = static_cast<float>(m_descSwapChainSupportDetails.Extent.height);
    viewport.minDepth   = 0.0f;
    viewport.maxDepth   = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent   = m_descSwapChainSupportDetails.Extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    //~ Set Vertex buffer
    const VkBuffer vertexBuffers[] = { m_vkVertexBuffer };
    constexpr VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_vkIndexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS, m_vkPipelineLayout, 0, 1,
        &m_pvkDescSets[m_nCurrentFrame], 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, m_nIndexCounts, 1, 0, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed recording command buffer");
}

void RenderManager::CreateBuffer(
        const VkDeviceSize          size,
        const VkBufferUsageFlags    usage,
        const VkMemoryPropertyFlags properties,
        VkBuffer&                   buffer,
        VkDeviceMemory              &bufferMemory
    ) const
{
    LOG_WARNING("[Helper] Attempting to creating buffer");
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size        = size;
    bufferInfo.usage       = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed creating vertex buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_vkDevice, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = Fox::FindMemoryType(m_vkPhysicalDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed allocating vertex buffer memory");

    vkBindBufferMemory(m_vkDevice, buffer, bufferMemory, 0);

    LOG_SUCCESS("[Helper] Created vertex buffer");
}

void RenderManager::CopyBufferData(
    const VkBuffer src,
    const VkBuffer dst,
    const VkDeviceSize size) const
{
    LOG_WARNING("Attempting to copy buffer data");
    const VkCommandBuffer commandBuffer = BeginSetupCommands();
    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    EndSetupCommands(commandBuffer);
    LOG_SUCCESS("Buffer data has been copied!");
}

void RenderManager::CreateSyncObjects()
{
    LOG_WARNING("Creating Thread (GPU Execution) locks");

    m_threadImageAvailableSemaphore.resize(Fox::MAX_FRAMES_IN_FLIGHT);
    m_threadInFlightFences         .resize(Fox::MAX_FRAMES_IN_FLIGHT);
    m_threadRenderFinishedSemaphore.resize(Fox::MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < Fox::MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr,
            &m_threadImageAvailableSemaphore[i]) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed creating image available semaphore");

        if (vkCreateSemaphore(m_vkDevice, &semaphoreInfo, nullptr,
            &m_threadRenderFinishedSemaphore[i]) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed creating finish semaphore");

        if (vkCreateFence(m_vkDevice, &fenceInfo, nullptr,
            &m_threadInFlightFences[i]) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed creating fences");
    }

    LOG_SUCCESS("GPU-thread protection locks are created");
}

void RenderManager::ReleaseSwapChain() const
{
    for (const auto m_vkSwapChainFramebuffer : m_vkSwapChainFramebuffers)
        vkDestroyFramebuffer(m_vkDevice, m_vkSwapChainFramebuffer, nullptr);

    for (const auto m_vkSwapChainImageView : m_vkSwapChainImageViews)
        vkDestroyImageView(m_vkDevice, m_vkSwapChainImageView, nullptr);

    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);
}

void RenderManager::RecreateSwapChain()
{
    m_bWindowResizeHandled = false;
    vkDeviceWaitIdle(m_vkDevice);
    ReleaseSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();
}

void RenderManager::UpdateUniformBuffer(const uint32_t imageIndex) const
{
    VERTEX_UNIFORM_DATA_DESC desc{};
    TestAnimation(m_nElapsedTime, desc.Transformation);

    desc.View = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    desc.Projection = glm::perspective(
        glm::radians(45.0f),
        m_descSwapChainSupportDetails.Extent.width
        / static_cast<float>(m_descSwapChainSupportDetails.Extent.height),
        0.1f, 10.0f
    );
    desc.Projection[1][1] *= -1.0f;

    desc.TotalElapsedTime = m_nElapsedTime;

    memcpy(m_ppUniformBuffersMapped[imageIndex], &desc, sizeof(desc));
}

void RenderManager::TestAnimation(const float deltaTime, glm::mat4& transform) const
{
    // Spin + pulse scale + orbit = good vibes
    const float angle     = deltaTime * glm::radians(45.0f);
    const float pulse     = 0.2f * sin(deltaTime * 2.0f);
    const float baseScale = 1.0f + pulse;

    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1));
    const glm::mat4 scaling  = glm::scale(glm::mat4(1.0f), glm::vec3(baseScale));
    const glm::mat4 orbit    = glm::translate(glm::mat4(1.0f), glm::vec3(sin(angle) * 0.5f, cos(angle) * 0.5f, 0.0f));

    transform = orbit * rotation * scaling;
}

void RenderManager::CreateImage(
    const uint32_t width,
    const uint32_t height,
    const VkFormat format,
    const VkImageTiling tiling,
    const VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageViewMemory
)
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_vkDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_vkDevice, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Fox::FindMemoryType(m_vkPhysicalDevice,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &imageViewMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(m_vkDevice, image, imageViewMemory, 0);
}

VkCommandBuffer RenderManager::BeginSetupCommands() const
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool        = m_vkCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void RenderManager::EndSetupCommands(const VkCommandBuffer commandBuffer) const
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(m_vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vkGraphicsQueue);

    vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

void RenderManager::TransitionImageLayout(
    VkImage image,
    VkFormat format,
    VkImageLayout oldLayout,
    VkImageLayout newLayout) const
{
    const VkCommandBuffer commandBuffer = BeginSetupCommands();
    EndSetupCommands(commandBuffer);
}
