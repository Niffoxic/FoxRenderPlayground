//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"
#include "Logger/Logger.h"
#include "ExceptionHandler/IException.h"

#include <stdexcept>


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
    vkDestroyDevice(m_vkDevice, nullptr);

#if defined(DEBUG) || defined(_DEBUG)
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
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
    CreateInstance();
    CreateSurface();

#if defined(DEBUG) || defined(_DEBUG)
    CreateDebugUtilsMessenger(m_vkInstance, m_vkDebugMessenger);
#endif

    SelectPhysicalDevice();
    CreateLogicalDevice();
    return true;
}

void RenderManager::CreateInstance()
{
    LOG_WARNING("Trying to create a new VkInstance instance");
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = m_pWinManager->GetWindowsTitle().c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0); // TODO: Get it directly from CMake can ya!!
    appInfo.pEngineName = "RenderEngine"; // TODO: This as well
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0); // TODO: umm hmm
    appInfo.apiVersion = VK_API_VERSION_1_4; // WE DO NOT CARE ABOUT ANYTHING LESS

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    constexpr auto requiredExtensions = Fox::vkRequiredInstanceExtensions;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

// Add Debug Extension
#if defined(DEBUG) || defined(_DEBUG)
    PrintAvailableInstanceExtensions();
    if (!CheckValidationLayerSupport()) THROW_EXCEPTION();

    createInfo.enabledLayerCount  = static_cast<uint32_t>(Fox::vkValidationLayers.size());
    createInfo.ppEnabledLayerNames = Fox::vkValidationLayers.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    PopulateDebugMessengerCreateInfo(debugCreateInfo);
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
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = m_pWinManager->GetWinHandle();
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
        if (IsDeviceSuitable(device))
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
    const QUEUE_FAMILY_INDEX_DESC desc = FindQueueFamily(m_vkPhysicalDevice);
    if (not desc.IsInitialized()) THROW_EXCEPTION_MSG("Not a suitable family desc");

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = desc.GraphicsFamily.value();
    queueCreateInfo.queueCount = 1;

    constexpr float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;
    createInfo.enabledLayerCount = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createInfo.enabledLayerCount = static_cast<uint32_t>(Fox::vkValidationLayers.size());
    createInfo.ppEnabledLayerNames = Fox::vkValidationLayers.data();
#endif

    if (vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create logical device!");

    vkGetDeviceQueue(m_vkDevice, desc.GraphicsFamily.value(), 0, &m_vkGraphicsQueue);
    LOG_SUCCESS("Vulkan Device Created!");
}
