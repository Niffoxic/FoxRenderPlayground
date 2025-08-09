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
    auto typeStr = "";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)     typeStr = "GENERAL";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  typeStr = "VALIDATION";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) typeStr = "PERFORMANCE";

    // Log according to severity
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        LOG_ERROR("[{}][Validation Error] {}", typeStr, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        LOG_WARNING("[{}][Validation Warning] {}", typeStr, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        LOG_INFO("[{}][Validation Info] {}", typeStr, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        LOG_PRINT("[{}][Validation Verbose] {}", typeStr, pCallbackData->pMessage);
    }

    return VK_FALSE;
}

bool FxInstance::Init()
{
    LOG_SCOPE("FxInstance Init", /*hasNextSibling=*/false);
    {
        m_pAllocator = m_descInstance.pAllocator;

        LOG_SCOPE("Fill App Info", /*hasNextSibling=*/true);
        {
            FillAppInfo(m_descInstance);
            LOG_SUCCESS("App='{}' v{} | Engine='{}' v{} | API v{}.{}.{}",
                        m_descInstance.AppName,
                        m_descInstance.AppVersion,
                        m_descInstance.EngineName,
                        m_descInstance.EngineVersion,
                        VK_VERSION_MAJOR(m_descInstance.ApiVersion),
                        VK_VERSION_MINOR(m_descInstance.ApiVersion),
                        VK_VERSION_PATCH(m_descInstance.ApiVersion));
        }
        LOG_SCOPE_END();

        LOG_SCOPE("Pick Extensions", /*hasNextSibling=*/true);
        {
            PickExtensions(m_descInstance);
        }
        LOG_SCOPE_END();

        LOG_SCOPE("Pick Layers", /*hasNextSibling=*/true);
        {
            PickLayers(m_descInstance);
        }
        LOG_SCOPE_END();

#if defined(_DEBUG) || defined(DEBUG)
        LOG_SCOPE("Prepare Debug Messenger", /*hasNextSibling=*/true);
        {
            FillDebugMessenger();
            LOG_SUCCESS("Debug messenger create info prepared");
        }
        LOG_SCOPE_END();
#endif

        LOG_SCOPE("Create VkInstance", /*hasNextSibling=*/true);
        {
            VkInstance instance = VK_NULL_HANDLE;
            const VkResult vr = vkCreateInstance(&m_infoVkInstance, m_pAllocator, &instance);
            if (vr != VK_SUCCESS)
            {
                LOG_ERROR("vkCreateInstance failed: VkResult={}", static_cast<int>(vr));
                THROW_EXCEPTION_MSG("Failed to create Vulkan instance");
            }

            // RAII for instance; capture allocator for proper destroy
            m_pInstance = FxPtr<VkInstance>(
                instance,
                [allocator = m_pAllocator](VkInstance ins)
                {
                    if (ins) vkDestroyInstance(ins, allocator);
                }
            );

            LOG_SUCCESS("VkInstance created");
        }
        LOG_SCOPE_END();

#if defined(_DEBUG) || defined(DEBUG)
        LOG_SCOPE("Create Debug Messenger", /*hasNextSibling=*/false);
        {
            CreateDebugMessenger();
        }
        LOG_SCOPE_END();
#endif
    }
    return true;
}


void FxInstance::Release()
{
    LOG_SCOPE("FxInstance Release", /*hasNextSibling=*/false);
    {
        m_pDebugMessenger.Reset();
        m_pInstance.Reset();
        LOG_SUCCESS("Destroyed instance and debug messenger (if any)");
    }
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
    const auto CreateDebugUtilsMessengerEXT =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_pInstance.Get(), "vkCreateDebugUtilsMessengerEXT"));

    if (!CreateDebugUtilsMessengerEXT)
    {
        LOG_WARNING("vkCreateDebugUtilsMessengerEXT not found (extension may be missing)");
        return;
    }

    VkDebugUtilsMessengerEXT debugger = VK_NULL_HANDLE;
    const VkResult vr = CreateDebugUtilsMessengerEXT(
        m_pInstance.Get(), &m_infoDebugMessenger, m_pAllocator, &debugger);

    if (vr != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to create Vulkan debug messenger");

    LOG_SUCCESS("Vulkan debug messenger created");

    m_pDebugMessenger = FxPtr<VkDebugUtilsMessengerEXT>(
        debugger,
        [instance = m_pInstance.Get(), allocator = m_pAllocator](VkDebugUtilsMessengerEXT dbg)
        {
            if (!dbg || !instance) return;
            const auto DestroyDebugUtilsMessengerEXT =
                reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (DestroyDebugUtilsMessengerEXT)
                DestroyDebugUtilsMessengerEXT(instance, dbg, allocator);
        }
    );
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
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, available.data());

    LOG_INFO("Instance extensions available: {}", extensionCount);

    // Requested extensions
    std::vector<std::string> desiredExtensions {
        "VK_KHR_surface",
        "VK_KHR_win32_surface",
    };
#if defined(_DEBUG) || defined(DEBUG)
    desiredExtensions.emplace_back("VK_EXT_debug_utils");
#endif

    std::unordered_map<std::string, bool> extensionMap;
    for (const auto& ext : desiredExtensions) extensionMap[ext] = false;

    m_ppEnabledExtensionNames.clear();
    m_ppEnabledExtensions = available; // cache for SupportsExtension

    for (const auto& [extensionName, specVersion] : available)
    {
        if (extensionMap.contains(extensionName))
        {
            m_ppEnabledExtensionNames.emplace_back(extensionName);
            LOG_SUCCESS("Enable ext: {}", extensionName);
            extensionMap[extensionName] = true;
        }
    }

    bool error = false;
    for (const auto& [name, found] : extensionMap)
    {
        if (!found) { error = true; LOG_ERROR("Missing required ext: {}", name); }
    }
    if (error) THROW_EXCEPTION_MSG("Required instance extensions not found");

    LOG_SUCCESS("All required instance extensions enabled");
}

void FxInstance::PickLayers(const FOX_INSTANCE_CREATE_DESC& desc)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> available(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, available.data());

    LOG_INFO("Instance layers available: {}", layerCount);

    std::vector<std::string> desiredLayers;
#if defined(_DEBUG) || defined(DEBUG)
    desiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
#endif

    std::unordered_map<std::string, bool> layerMap;
    for (const auto& l : desiredLayers) layerMap[l] = false;

    m_ppEnabledLayerNames.clear();
    m_ppEnabledLayers = available; // cache for SupportsLayer

    for (const auto& layer : available)
    {
        const std::string name = layer.layerName;
        if (layerMap.contains(name))
        {
            m_ppEnabledLayerNames.emplace_back(layer.layerName);
            LOG_SUCCESS("Enable layer: {}", name);
            layerMap[name] = true;
        }
    }

    bool error = false;
    for (const auto& [name, found] : layerMap)
    {
        if (!found) { error = true; LOG_ERROR("Missing required layer: {}", name); }
    }
    if (error) THROW_EXCEPTION_MSG("Required instance layers not found");

    LOG_SUCCESS("All required instance layers enabled");
}

void FxInstance::FillInstanceCreateInfo()
{
    m_infoVkInstance = {};
    m_infoVkInstance.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    m_infoVkInstance.pApplicationInfo        = &m_infoVkApp;
    m_infoVkInstance.enabledExtensionCount   = static_cast<uint32_t>(m_ppEnabledExtensionNames.size());
    m_infoVkInstance.ppEnabledExtensionNames = m_ppEnabledExtensionNames.data();
    m_infoVkInstance.enabledLayerCount       = static_cast<uint32_t>(m_ppEnabledLayerNames.size());
    m_infoVkInstance.ppEnabledLayerNames     = m_ppEnabledLayerNames.data();
#if defined(_DEBUG) || defined(DEBUG)
    m_infoVkInstance.pNext                   = &m_infoDebugMessenger;
#else
    m_infoVkInstance.pNext                   = nullptr;
#endif
    m_infoVkInstance.flags                   = 0;

    LOG_INFO("InstanceCreateInfo: extCount={}, layerCount={}",
             m_infoVkInstance.enabledExtensionCount,
             m_infoVkInstance.enabledLayerCount);
}
