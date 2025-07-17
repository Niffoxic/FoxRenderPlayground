//
// Created by niffo on 7/17/2025.
//

#ifndef DEFINEVULKAN_H
#define DEFINEVULKAN_H

#include <array>
#include <vector>
#include <vulkan/vulkan.h>
#include <optional>

#include "ExceptionHandler/WindowException.h"
#include "Logger/Logger.h"

#pragma region CONFIGURATION
namespace Fox
{
    inline constexpr std::array<const char*, 1> vkValidationLayers
    {
        "VK_LAYER_KHRONOS_validation"
    };

    constexpr auto GetRequiredInstanceExtensions()
    {
#if defined(_DEBUG) || defined(DEBUG)
        return std::to_array<const char*>({
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        });
#else
        return std::to_array<const char*>({
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        });
#endif
    }

    inline constexpr auto vkRequiredInstanceExtensions = GetRequiredInstanceExtensions();
}

#pragma endregion

#pragma region CUSTOM_DESCRIPTION

typedef struct QUEUE_FAMILY_INDEX_DESC
{
    std::optional<uint32_t> GraphicsFamily;

    bool IsInitialized() const
    {
        return GraphicsFamily.has_value();
    }
}QUEUE_FAMILY_INDEX_DESC;

#pragma endregion

#pragma region VULKAN_HELPERS
inline void PrintAvailableInstanceExtensions()
{
    uint32_t extensionCount = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr) != VK_SUCCESS)
    {
        THROW_WINDOW_EXCEPTION();
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data()) != VK_SUCCESS)
    {
        THROW_WINDOW_EXCEPTION();
    }

    LOG_INFO("Available extensions:");
    for (const auto&[extensionName, specVersion] : extensions)
    {
        LOG_PRINT("\t{}", extensionName);
    }
}

inline QUEUE_FAMILY_INDEX_DESC FindQueueFamily(VkPhysicalDevice device)
{
    QUEUE_FAMILY_INDEX_DESC desc{};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int graphicsSupportedFamily = 0;
    for (const auto& queueFamily : queueFamilies)
    {
        if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && not desc.IsInitialized())
            desc.GraphicsFamily = graphicsSupportedFamily;

        graphicsSupportedFamily++;
    }

    return desc;
}

inline bool IsDeviceSuitable(VkPhysicalDevice device)
{
    const QUEUE_FAMILY_INDEX_DESC desc = FindQueueFamily(device);
    return desc.IsInitialized();
}

#pragma endregion

#pragma region DEBUG_IMPL

inline VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(
        instance,
        "vkCreateDebugUtilsMessengerEXT"));

    if (func != nullptr) return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator)
{
    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(
            instance,
            "vkDestroyDebugUtilsMessengerEXT"
            )
        );

    if (func != nullptr) func(instance, debugMessenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    const char* typeStr = "";
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)      typeStr = "GENERAL";
    else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) typeStr = "VALIDATION";
    else if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) typeStr = "PERFORMANCE";

    const char* message = pCallbackData->pMessage;

    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        LOG_PRINT("[VK][{}]: {}", typeStr, message);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        LOG_INFO("[VK][{}]: {}", typeStr, message);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        LOG_WARNING("[VK][{}]: {}", typeStr, message);
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        LOG_ERROR("[VK][{}]: {}", typeStr, message);
        break;

    default:
        LOG_PRINT("[VK][UNKNOWN]: {}", message);
        break;
    }

    return VK_FALSE;
}

inline void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugCallback;
}

inline bool CheckValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : Fox::vkValidationLayers)
    {
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) return false;
    }
    LOG_SUCCESS("Validation Layers Found!");
    return true;
}

inline void CreateDebugUtilsMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debugMessenger)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    PopulateDebugMessengerCreateInfo(createInfo);
    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        THROW_EXCEPTION_MSG("Failed to set up debug messenger!");
}


#pragma endregion

#endif //DEFINEVULKAN_H
