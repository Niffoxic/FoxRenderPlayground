//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"
#include "Logger/Logger.h"

#include <stdexcept>
#include "ExceptionHandler/IException.h"

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
#if defined(DEBUG) || defined(_DEBUG)
    DestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
#endif

    if (m_vkInstance)
    {
        vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = nullptr;
    }
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

#if defined(DEBUG) || defined(_DEBUG)
    CreateDebugUtilsMessenger(m_vkInstance, m_vkDebugMessenger);
#endif
    return true;
}

void RenderManager::CreateInstance()
{
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
