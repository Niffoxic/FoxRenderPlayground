//
// Created by niffo on 8/4/2025.
//

#include "FxInstance.h"
#include "ExceptionHandler/IException.h"
#include "Logger/Logger.h"

#include <unordered_map>

bool FxInstance::Init()
{
    m_pAllocator = m_descInstance.pAllocator;

    FillAppInfo(m_descInstance);
    PickExtensions(m_descInstance);
    PickLayers(m_descInstance);
    FillInstanceCreateInfo();

    VkInstance instance;
    const VkResult result = vkCreateInstance(&m_infoVkInstance, nullptr, &instance);
    if (result != VK_SUCCESS)
    {
        LOG_ERROR("vkCreateInstance failed with error code: {}", static_cast<int>(result));
        THROW_EXCEPTION_MSG("Failed to create Vulkan instance");
    }

    m_pInstance = FxPtr<VkInstance>(instance,
        [allocator = m_pAllocator](const VkInstance ins)
        {
            vkDestroyInstance(ins,
            nullptr);
        });

    return true;
}

void FxInstance::Release()
{
    if (!m_pInstance.IsValid())
        return;

#if defined(_DEBUG) || defined(DEBUG)
    DestroyDebugMessenger();
#endif

    m_pInstance.Reset();
}

void FxInstance::Describe(const FOX_INSTANCE_CREATE_DESC &desc)
{
    m_descInstance = desc;
}

bool FxInstance::SupportsExtension(const char* extensinName) const
{
    for (const auto& [extensionName, specVersion] : m_ppEnabledExtensions)
        if (std::strcmp(extensionName, extensinName) == 0)
            return true;
    return false;
}

bool FxInstance::SupportsLayer(const char* layerName) const
{
    for (const auto& ext : m_ppEnabledLayers)
        if (std::strcmp(ext.layerName, layerName) == 0)
            return true;
    return false;
}

#if defined(_DEBUG) || defined(DEBUG)
void FxInstance::SetupDebugMessenger()
{

}
void FxInstance::DestroyDebugMessenger()
{
    m_pDebugMessenger.Reset();
}
#endif

void FxInstance::FillAppInfo(const FOX_INSTANCE_CREATE_DESC &desc)
{
    m_infoVkApp = {};
    m_infoVkApp.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_infoVkApp.pApplicationName   = desc.AppName.c_str();
    m_infoVkApp.pEngineName        = desc.EngineName.c_str();
    m_infoVkApp.apiVersion         = desc.ApiVersion;
    m_infoVkApp.engineVersion      = desc.EngineVersion;
    m_infoVkApp.applicationVersion = desc.AppVersion;
}

void FxInstance::PickExtensions(const FOX_INSTANCE_CREATE_DESC& desc)
{
    uint32_t extensionCount;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    LOG_INFO("Extension Found: {}", extensionCount);
    m_ppEnabledExtensions.resize(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, m_ppEnabledExtensions.data());

    // Requested extensions
    const std::vector<std::string> desiredExtensions
    {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_EXT_debug_utils"
    };
    std::unordered_map<std::string, bool> extensionMap;
    for (const auto& extension : desiredExtensions) extensionMap[extension] = false;

    m_ppEnabledExtensionNames.clear();
    for (const auto& [extensionName, specVersion] : m_ppEnabledExtensions)
    {
        if (const std::string name = extensionName; extensionMap.contains(name))
        {
            m_ppEnabledExtensionNames.emplace_back(extensionName);
            LOG_INFO("Extension Found: {}", name);
            extensionMap[name] = true;
        }
    }
    bool error = false;
    for (const auto& [name, flag]: extensionMap)
    {
        if (not flag)
        {
            error = true;
            LOG_ERROR("Extension not found: {}", name);
        }
    }
    if (error) THROW_EXCEPTION_MSG("Failed to required extensions");
    LOG_SUCCESS("Found All Extension needed");
}

void FxInstance::PickLayers(const FOX_INSTANCE_CREATE_DESC& desc)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    LOG_INFO("Layers Found: {}", layerCount);
    m_ppEnabledLayers.resize(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, m_ppEnabledLayers.data());

    // Requested layers
    const std::vector<std::string> desiredLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };
    std::unordered_map<std::string, bool> layerMap;
    for (const auto& layer : desiredLayers) layerMap[layer] = false;

    m_ppEnabledLayerNames.clear();
    for (const auto& layer: m_ppEnabledLayers)
    {
        if (const std::string name = layer.layerName; layerMap.contains(name))
        {
            m_ppEnabledLayerNames.emplace_back(layer.layerName);
            LOG_INFO("Layer Found: {}", layer.layerName);
            layerMap[name] = true;
        }
    }

    bool error = false;
    for (const auto& [name, flag]: layerMap)
    {
        if (not flag)
        {
            error = true;
            LOG_ERROR("Layer not found: {}", name);
        }
    }

    if (error) THROW_EXCEPTION_MSG("Failed to required layers");
    LOG_SUCCESS("Found All Layer needed");
}

void FxInstance::FillInstanceCreateInfo()
{
    m_infoVkInstance = {};
    m_infoVkInstance.sType          = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    m_infoVkInstance.enabledExtensionCount = m_ppEnabledExtensionNames.size();
    m_infoVkInstance.enabledLayerCount  = m_ppEnabledLayerNames.size();
    m_infoVkInstance.pApplicationInfo = &m_infoVkApp;
    m_infoVkInstance.ppEnabledExtensionNames = m_ppEnabledExtensionNames.data();
    m_infoVkInstance.ppEnabledLayerNames = m_ppEnabledLayerNames.data();
    m_infoVkInstance.flags = 0;
    m_infoVkInstance.pNext = nullptr;

    LOG_INFO("---- Vulkan Instance Create Info ----");

    LOG_INFO("Application Name     : {}", m_infoVkApp.pApplicationName ? m_infoVkApp.pApplicationName : "(null)");
    LOG_INFO("Engine Name          : {}", m_infoVkApp.pEngineName ? m_infoVkApp.pEngineName : "(null)");
    LOG_INFO("Application Version  : {}.{}.{}",
        VK_VERSION_MAJOR(m_infoVkApp.applicationVersion),
        VK_VERSION_MINOR(m_infoVkApp.applicationVersion),
        VK_VERSION_PATCH(m_infoVkApp.applicationVersion));

    LOG_INFO("Engine Version       : {}.{}.{}",
        VK_VERSION_MAJOR(m_infoVkApp.engineVersion),
        VK_VERSION_MINOR(m_infoVkApp.engineVersion),
        VK_VERSION_PATCH(m_infoVkApp.engineVersion));

    LOG_INFO("API Version          : {}.{}.{}",
        VK_VERSION_MAJOR(m_infoVkApp.apiVersion),
        VK_VERSION_MINOR(m_infoVkApp.apiVersion),
        VK_VERSION_PATCH(m_infoVkApp.apiVersion));


    LOG_INFO("Enabled Extensions   : {}", m_ppEnabledExtensionNames.size());
    for (const char* ext : m_ppEnabledExtensionNames)
        LOG_INFO("  - {}", ext);

    LOG_INFO("Enabled Layers       : {}", m_ppEnabledLayerNames.size());
    for (const char* layer : m_ppEnabledLayerNames)
        LOG_INFO("  - {}", layer);

    LOG_INFO("-------------------------------------");
}
