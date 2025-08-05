//
// Created by niffo on 8/4/2025.
//

#include "FxInstance.h"
#include "ExceptionHandler/IException.h"
#include "Logger/Logger.h"

#include <unordered_map>

#include "Common/DefineVulkan.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT              messageType,
    const VkDebugUtilsMessengerCallbackDataEXT*  pCallbackData,
    void*                                        pUserData)
{
    LOG_WARNING("[Validation] {}", pCallbackData->pMessage);
    return VK_FALSE;
}

bool FxInstance::Init()
{
    m_pAllocator = m_descInstance.pAllocator;

    FillAppInfo(m_descInstance);
    PickExtensions(m_descInstance);
    PickLayers(m_descInstance);
#if defined(_DEBUG) || defined(DEBUG)
    FillDebugMessenger();
#endif
    FillInstanceCreateInfo();

    VkInstance instance;
    if (const auto result = vkCreateInstance(&m_infoVkInstance, nullptr, &instance) != VK_SUCCESS)
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

#if defined(_DEBUG) || defined(DEBUG)
    CreateDebugMessenger();
#endif

    return true;
}

void FxInstance::Release()
{
    m_pDebugMessenger.Reset();
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

void FxInstance::FillDebugMessenger()
{
    m_infoDebugMessenger = {};
    m_infoDebugMessenger.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    m_infoDebugMessenger.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    m_infoDebugMessenger.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    m_infoDebugMessenger.pfnUserCallback = DebugCallback;
}

void FxInstance::CreateDebugMessenger()
{
    const auto CreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(
            m_pInstance.Get(), "vkCreateDebugUtilsMessengerEXT"
        )
    );

    if (CreateDebugUtilsMessenger)
    {
        VkDebugUtilsMessengerEXT debugger;
        if (Fox::CreateDebugUtilsMessengerEXT(m_pInstance.Get(),
            &m_infoDebugMessenger, m_pAllocator, &debugger) != VK_SUCCESS)
            THROW_EXCEPTION_MSG("Failed to create vulkan debugger");

        LOG_SUCCESS("Created Debugger!");

        m_pDebugMessenger = FxPtr<VkDebugUtilsMessengerEXT>(
            debugger,
            [instance = m_pInstance.Get(), allocator = m_pAllocator](const VkDebugUtilsMessengerEXT debug)
            {
                const auto DestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>
                (
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
                );

                if (DestroyDebugUtilsMessengerEXT && debug)
                {
                    DestroyDebugUtilsMessengerEXT(instance, debug, allocator);
                }
            });
    }
}

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
    std::vector<std::string> desiredExtensions
    {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
        "VK_EXT_debug_utils"
    };

#if defined(_DEBUG) || defined(DEBUG)
    desiredExtensions.emplace_back("VK_EXT_debug_utils");
#endif

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
    std::vector<std::string> desiredLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

#if defined(_DEBUG) || defined(DEBUG)
    desiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
#endif

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
    m_infoVkInstance.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    m_infoVkInstance.enabledExtensionCount   = m_ppEnabledExtensionNames.size();
    m_infoVkInstance.enabledLayerCount       = m_ppEnabledLayerNames.size();
    m_infoVkInstance.pApplicationInfo        = &m_infoVkApp;
    m_infoVkInstance.ppEnabledExtensionNames = m_ppEnabledExtensionNames.data();
    m_infoVkInstance.ppEnabledLayerNames     = m_ppEnabledLayerNames.data();
    m_infoVkInstance.flags                   = 0;
#if defined(_DEBUG) || defined(DEBUG)
    m_infoVkInstance.pNext                   = &m_infoDebugMessenger;
#else
    m_infoVkInstance.pNext                   = nullptr;
#endif
}
