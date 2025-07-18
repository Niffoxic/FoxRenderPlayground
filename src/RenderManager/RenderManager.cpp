//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"

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

bool RenderManager::OnRelease()
{
    //~ Clean Swap chain
    for (auto& view: m_vkSwapChainImageViews)
    {
        vkDestroyImageView(m_vkDevice, view, nullptr);
    }

    vkDestroySwapchainKHR(m_vkDevice, m_vkSwapChain, nullptr);

    //~ Clean Vulkan resources
    vkDestroyDevice(m_vkDevice, nullptr);

#if defined(DEBUG) || defined(_DEBUG)
    Fox::DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
#endif

    vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
    vkDestroyInstance(m_vkInstance, nullptr);

    return true;
}

void RenderManager::OnFrameBegin()
{

}

void RenderManager::OnFramePresent()
{

}

void RenderManager::OnFrameEnd()
{

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

void RenderManager::CreateRenderPipeline()
{

}
