//
// Created by niffo on 7/17/2025.
//

#ifndef DEFINEVULKAN_H
#define DEFINEVULKAN_H

#define NOMINMAX
#include <array>
#include <vector>
#include <vulkan/vulkan.h>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>

#include "WindowsManager/WindowsManager.h"
#include "ExceptionHandler/WindowException.h"
#include "Logger/Logger.h"

#pragma region CUSTOM_DESCRIPTION

typedef struct QUEUE_FAMILY_INDEX_DESC
{
    std::optional<uint32_t> GraphicsFamily;
    std::optional<uint32_t> PresentFamily;

    _fox_Return_safe
    bool IsInitialized() const
    {
        return
        GraphicsFamily.has_value() &&
        PresentFamily.has_value();
    }
}QUEUE_FAMILY_INDEX_DESC;

typedef struct SWAP_CHAIN_SUPPORT_DESC
{
    VkSurfaceCapabilitiesKHR Capabilities;
    std::vector<VkSurfaceFormatKHR> Formats;
    std::vector<VkPresentModeKHR> PresentModes;

    _fox_Return_safe
    bool IsInitialized() const
    {
        return not Formats.empty() && not PresentModes.empty();
    }

}SWAP_CHAIN_SUPPORT_DESC;

#pragma endregion

namespace Fox
{
#pragma region CONFIGURATION
    inline constexpr std::array<const char*, 1> vkDeviceExtensions
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

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

    inline QUEUE_FAMILY_INDEX_DESC FindQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        LOG_WARNING("Finding Queue Family");
        QUEUE_FAMILY_INDEX_DESC desc{};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int queueIndex = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, queueIndex, surface, &presentSupport);

            if (not desc.IsInitialized() && presentSupport)
            {
                desc.PresentFamily = queueIndex;
                LOG_INFO("Found Present Family at: {}", desc.PresentFamily.value());
            }
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && not desc.IsInitialized())
            {
                desc.GraphicsFamily = queueIndex;
                LOG_INFO("Found Graphics Family at: {}", desc.GraphicsFamily.value());
            }
            if (desc.IsInitialized())
            {
                LOG_SUCCESS("Found Suitable Queue Family");
                return desc;
            }
            queueIndex++;
        }
        return desc;
    }

    inline bool CheckDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount{ 0 };
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

        std::set<std::string> requiredExtensions;
        std::ranges::transform(extensions, std::inserter(requiredExtensions, requiredExtensions.end()),
            [](const VkExtensionProperties& ext) { return std::string(ext.extensionName); });


        for (const auto&[extensionName, specVersion] : extensions)
        {
            requiredExtensions.erase(extensionName);
        }

        return requiredExtensions.empty();
    }

    _fox_Return_safe _fox_Pre_satisfies_(device != VK_NULL_HANDLE) _fox_Success_(return != 0)
    inline uint32_t FindMemoryType(VkPhysicalDevice device, const uint32_t filter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(device, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((filter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

#if defined(DEBUG) || defined(_DEBUG)
        __debugbreak();
#else
        THROW_EXCEPTION_MSG("Failed to Find memory type: crucial for creating vertex buffer");
#endif
        return 0;
    }

    #pragma endregion

    #pragma region DEBUG_IMPL

    _fox_Return_safe
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
            try {
                THROW_EXCEPTION_FMT("[VK][{}]: {}", typeStr, message);
            }
            catch (const IException& e) {
                OutputDebugStringA(e.what());
                std::string msg = e.what();
                LOG_ERROR("{}", msg);
            }
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

    inline SWAP_CHAIN_SUPPORT_DESC QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SWAP_CHAIN_SUPPORT_DESC desc{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &desc.Capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount)
        {
            desc.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, desc.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount)
        {
            desc.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                device,
                surface,
                &presentModeCount,
                desc.PresentModes.data());
        }

        return desc;
    }

    inline VkSurfaceFormatKHR SelectSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& formats)
    {
        for (const auto& format : formats)
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return format;
        return formats[0];
    }

    inline VkPresentModeKHR SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
    {
        for (const auto& mode : presentModes)
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) return mode;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    inline VkExtent2D SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities, const WindowsManager* win)
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }

        VkExtent2D extent
        {
            win->GetWindowSize().Width,
            win->GetWindowSize().Height
        };

        extent.width = std::clamp(
            extent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width
        );

        extent.height = std::clamp(
            extent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height
        );

        return extent;
    }

    inline bool CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : vkValidationLayers)
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

    inline bool IsDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
    {
        const QUEUE_FAMILY_INDEX_DESC desc = FindQueueFamily(device, surface);

        const bool extensionFound = CheckDeviceExtensionSupport(device);
        bool swapChainAdequate = false;

        if (extensionFound)
        {
            SWAP_CHAIN_SUPPORT_DESC swapChainDesc = QuerySwapChainSupport(device, surface);
            swapChainAdequate = swapChainDesc.IsInitialized();
        }

        return desc.IsInitialized() && swapChainAdequate && extensionFound;
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

}

