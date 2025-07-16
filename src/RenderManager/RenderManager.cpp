//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"

#include <stdexcept>
#include <iostream>     // TODO: Bruhhh write a basic logger at least bruhhhhhhhhhh

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

void RenderManager::PrintAvailableInstanceExtensions()
{
    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get vulkan extension count.");
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to get vulkan extension.");
    }

    std::cout << "Available extensions:\n";
    for (const auto& extension : extensions)
    {
        std::cout << "\t" << extension.extensionName << "\n";
    }
}

std::vector<const char *> RenderManager::GetRequiredInstanceExtensions()
{
    return
    {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
}

bool RenderManager::InitVulkan()
{
    CreateInstance();
    return true;
}

void RenderManager::CreateInstance()
{

#if defined(DEBUG) || defined(_DEBUG)
    PrintAvailableInstanceExtensions();
#endif

    const auto requiredExtensions = GetRequiredInstanceExtensions();
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
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledLayerNames = requiredExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &m_vkInstance) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create instance.");
    }

#if defined(ENABLE_TERMINAL)
    std::cout << "Vulkan Instance Created!" << std::endl;
#endif
}
