//
// Created by niffo on 8/4/2025.
//

#include "FxInstance.h"
#include "ExceptionHandler/IException.h"
#include "Logger/Logger.h"

bool FxInstance::Init()
{
    m_pAllocator = m_descInstance.pAllocator;

    //~ Fill descriptions
    FillAppInfo   (m_descInstance);
    PickExtensions(m_descInstance);
    PickLayers    (m_descInstance);
    FillInstanceCreateInfo();

    //~ Create Instance
    VkInstance instance = VK_NULL_HANDLE;

    if (vkCreateInstance(&m_infoVkInstance, m_pAllocator, &instance) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to initialize Vulkan Instance");

    m_pInstance = FxPtr<VkInstance>(
        instance,
        [allocator = m_pAllocator](const VkInstance ins)
        {
            vkDestroyInstance(ins, allocator);
        });

#if defined(_DEBUG) || defined(DEBUG)
    SetupDebugMessenger();
#endif

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
    for (const auto& [extensionName, specVersion] : m_availableExtensions)
        if (std::strcmp(extensionName, extensinName) == 0)
            return true;
    return false;
}

bool FxInstance::SupportsLayer(const char* layerName) const
{
    for (const auto& ext : m_availableLayers)
        if (std::strcmp(ext.layerName, layerName) == 0)
            return true;
    return false;
}

#if defined(_DEBUG) || defined(DEBUG)
void FxInstance::SetupDebugMessenger()
{
    if (!m_pInstance.IsValid())
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                    VkDebugUtilsMessageTypeFlagsEXT type,
                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                    void*) -> VkBool32
    {
        const std::string msg = pCallbackData->pMessage;

        if (msg.find("SteamOverlayVulkanLayer64.json") != std::string::npos)
        {
            LOG_WARNING("Fix this manually please: {}", msg);
            return VK_FALSE;
        }

        switch (severity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            LOG_INFO("[VK-Verbose] {}", msg);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            LOG_WARNING("[VK-Warning] {}", msg);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            LOG_ERROR("[VK-Error] {}", msg);
            // THROW_EXCEPTION_FMT("[Validation Error] {}", msg);
            break;
        default:
            LOG_PRINT("[VK] {}", msg);
            break;
        }

        return VK_FALSE;
    };

    const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(m_pInstance.Get(), "vkCreateDebugUtilsMessengerEXT"));

    if (vkCreateDebugUtilsMessengerEXT)
    {
        VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
        if (vkCreateDebugUtilsMessengerEXT(m_pInstance.Get(), &createInfo, m_pAllocator, &messenger) == VK_SUCCESS)
        {
            m_pDebugMessenger = FxPtr<VkDebugUtilsMessengerEXT>(
                messenger,
                [inst = m_pInstance.Get(), allocator = m_pAllocator](VkDebugUtilsMessengerEXT m) {
                    const auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                        vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT"));
                    if (destroy)
                        destroy(inst, m, allocator);
                });
        }
    }
}
void FxInstance::DestroyDebugMessenger()
{
    m_pDebugMessenger.Reset();
}
#endif

void FxInstance::FillAppInfo(const FOX_INSTANCE_CREATE_DESC &desc)
{
    m_infoVkApp = {};
    m_infoVkApp.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    m_infoVkApp.pApplicationName    = desc.AppName.c_str();
    m_infoVkApp.applicationVersion  = desc.AppVersion;
    m_infoVkApp.pEngineName         = desc.EngineName.c_str();
    m_infoVkApp.engineVersion       = desc.EngineVersion;
    m_infoVkApp.apiVersion          = desc.ApiVersion;
}

void FxInstance::PickExtensions(const FOX_INSTANCE_CREATE_DESC& desc)
{
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    m_ppEnabledExtensions.clear();

    for (const char* requested : desc.EnabledExtensionNames)
    {
        bool found = false;
        for (const auto& ext : availableExtensions)
        {
            if (std::strcmp(requested, ext.extensionName) == 0)
            {
                found = true;
                break;
            }
        }

        if (found) m_ppEnabledExtensions.push_back(requested);
        else THROW_EXCEPTION_FMT("[FxInstance] Skipping unsupported extension: {}", requested);

    }

#if defined(_DEBUG) || defined(DEBUG)
    // Add debug utils extension only if supported
    const char* debugExt = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    for (const auto& ext : availableExtensions)
    {
        if (std::strcmp(debugExt, ext.extensionName) == 0)
        {
            m_ppEnabledExtensions.push_back(debugExt);
            break;
        }
    }
#endif
}

void FxInstance::PickLayers(const FOX_INSTANCE_CREATE_DESC& desc)
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    m_ppEnabledLayers.clear();

    for (const char* requested : desc.EnabledLayerNames)
    {
        bool found = false;
        for (const auto& layer : availableLayers)
        {
            if (std::strcmp(requested, layer.layerName) == 0)
            {
                found = true;
                break;
            }
        }
        if (found) m_ppEnabledLayers.push_back(requested);
        else THROW_EXCEPTION_FMT("[FxInstance] Skipping unsupported layer: {}", requested);
    }

#if defined(_DEBUG) || defined(DEBUG)
    for (const auto& layer : availableLayers)
    {
        if (const auto validationLayer = "VK_LAYER_KHRONOS_validation";
            std::strcmp(validationLayer, layer.layerName) == 0)
        {
            m_ppEnabledLayers.push_back(validationLayer);
            break;
        }
    }
#endif
}

void FxInstance::FillInstanceCreateInfo()
{
    m_infoVkInstance = {};
    m_infoVkInstance.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    m_infoVkInstance.pApplicationInfo = &m_infoVkApp;
    m_infoVkInstance.enabledExtensionCount = static_cast<uint32_t>(m_ppEnabledExtensions.size());
    m_infoVkInstance.ppEnabledExtensionNames = m_ppEnabledExtensions.data();
    m_infoVkInstance.enabledLayerCount = static_cast<uint32_t>(m_ppEnabledLayers.size());
    m_infoVkInstance.ppEnabledLayerNames = m_ppEnabledLayers.data();

#if defined(_DEBUG) || defined(DEBUG)
    // Inject early debug utils if needed
    static VkDebugUtilsMessengerCreateInfoEXT earlyDebugCreate = {};
    earlyDebugCreate.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    earlyDebugCreate.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    earlyDebugCreate.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    earlyDebugCreate.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT,
                                          VkDebugUtilsMessageTypeFlagsEXT,
                                          const VkDebugUtilsMessengerCallbackDataEXT* pData,
                                          void*) -> VkBool32
    {
        LOG_WARNING("[Validation] {}", pData->pMessage);
        return VK_FALSE;
    };
    m_infoVkInstance.pNext = &earlyDebugCreate;
#else
    m_infoVkInstance.pNext = nullptr;
#endif
}
