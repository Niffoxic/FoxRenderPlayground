//
// Created by niffo on 8/9/2025.
//

#include "FxInstance.h"
#include "FxPhysicalDevice.h"
#include <algorithm>
#include <cstring>
#include <limits>


FxPhysicalDevice::~FxPhysicalDevice()
{
    Release();
}

void FxPhysicalDevice::Describe(const FX_PD_SELECTION_POLICY_DESC &desc)
{
    m_policy = desc;
}

void FxPhysicalDevice::AttachInstance(const FxInstance &instance, const VkSurfaceKHR surface)
{
    m_pInstance = &instance;

    if (surface != VK_NULL_HANDLE)
    {
        // Attach destructor that uses the *instance*; release before destroying VkInstance.
        VkInstance instHandle = m_pInstance ? m_pInstance->GetInstance() : VK_NULL_HANDLE;
        m_pSurface.Reset(surface, [inst = instHandle](const VkSurfaceKHR s)
        {
            if (inst != VK_NULL_HANDLE && s != VK_NULL_HANDLE)
                vkDestroySurfaceKHR(inst, s, nullptr);
        });
    }
}

void FxPhysicalDevice::DetachInstance()
{
    m_pInstance = nullptr;
}

void FxPhysicalDevice::SetSurface(const VkSurfaceKHR surface)
{
    VkInstance instHandle = m_pInstance ? m_pInstance->GetInstance() : VK_NULL_HANDLE;
    m_pSurface.Reset(surface, [inst = instHandle](const VkSurfaceKHR s)
    {
        if (inst != VK_NULL_HANDLE && s != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(inst, s, nullptr);
    });
}

bool FxPhysicalDevice::Init()
{
    LOG_SCOPE("Physical Device Init", /*hasNextSibling=*/false);
    {
        // Preconditions
        LOG_SCOPE("Preconditions", /*hasNextSibling=*/true);
        {
            if (!m_pInstance)
            {
                LOG_ERROR("No FxInstance attached");
                LOG_SCOPE_END();
                return false;
            }

            const VkSurfaceKHR surf = m_pSurface.Get();
            if (m_policy.RequireSwapChain && surf == VK_NULL_HANDLE)
            {
                LOG_ERROR("RequireSwapChain=true but no surface was provided");
                LOG_SCOPE_END();
                return false;
            }
            LOG_SUCCESS("Preconditions OK");
        }
        LOG_SCOPE_END();

        // Enumerate devices
        LOG_SCOPE("Enumerate Physical Devices", /*hasNextSibling=*/true);
        {
            if (!EnumeratePhysicalDevices())
            {
                LOG_ERROR("vkEnumeratePhysicalDevices failed");
                LOG_SCOPE_END();
                return false;
            }
            LOG_SUCCESS("Enumeration OK ({} device(s))", static_cast<int>(m_ppAllDevices.size()));
        }
        LOG_SCOPE_END();

        // Pick best device
        LOG_SCOPE("Pick Best Device", /*hasNextSibling=*/true);
        {
            const int best = PickBestDeviceIndex();
            if (best < 0)
            {
                LOG_ERROR("No suitable physical device found");
                LOG_SCOPE_END();
                return false;
            }

            const VkPhysicalDevice picked = m_ppAllDevices[static_cast<size_t>(best)];
            m_pPhysicalDevice.Reset(picked, nullptr); // physical device has no destructor
            LOG_SUCCESS("Selected device index: {}", best);
        }
        LOG_SCOPE_END();

        // Cache basics (props/mem/features/exts)
        LOG_SCOPE("Cache Device Basics", /*hasNextSibling=*/true);
        {
            CacheDeviceBasics();
            LOG_SUCCESS("Cached properties, memory, features, and available extensions");
        }
        LOG_SCOPE_END();

        // Queues
        LOG_SCOPE("Find Queue Families", /*hasNextSibling=*/true);
        {
            if (!FindQueueFamilies())
            {
                LOG_ERROR("Required queue families not satisfied");
                LOG_SCOPE_END();
                return false;
            }
            LOG_SUCCESS("Queues -> G={}, C={}, T={}, P={}",
                        m_qfIndices.Graphics, m_qfIndices.Compute,
                        m_qfIndices.Transfer, m_qfIndices.Present);
        }
        LOG_SCOPE_END();

        // Extensions
        LOG_SCOPE("Resolve Extensions", /*hasNextSibling=*/true);
        {
            if (!ResolveExtensions())
            {
                LOG_ERROR("Failed to resolve required/optional device extensions");
                LOG_SCOPE_END();
                return false;
            }
            LOG_SUCCESS("Enabled {} device extension(s)",
                        static_cast<int>(m_ppEnabledDeviceExtensions.size()));
        }
        LOG_SCOPE_END();

        // Core features
        LOG_SCOPE("Validate Required Core Features", /*hasNextSibling=*/true);
        {
            if (!ValidateRequiredCoreFeatures())
            {
                LOG_ERROR("Device lacks required core features");
                LOG_SCOPE_END();
                return false;
            }
            LOG_SUCCESS("All required core features supported");
        }
        LOG_SCOPE_END();

        // Present support sanity (if WSI)
        LOG_SCOPE("Validate Present Support (WSI)", /*hasNextSibling=*/false);
        {
            const VkSurfaceKHR surf = m_pSurface.Get();
            if (m_policy.RequireSwapChain && surf != VK_NULL_HANDLE)
            {
                const uint32_t qIndex = static_cast<uint32_t>(
                    m_qfIndices.Present >= 0 ? m_qfIndices.Present : m_qfIndices.Graphics);

                VkBool32 supported = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_pPhysicalDevice.Get(), qIndex, surf, &supported);
                if (!supported)
                {
                    LOG_ERROR("Queue family {} does not support presentation", qIndex);
                    LOG_SCOPE_END();
                    return false;
                }
            }
            LOG_SUCCESS("Present support OK (or not required)");
        }
        LOG_SCOPE_END();
    }

    LOG_SUCCESS("Physical Device Initialized");
    return true;
}

void FxPhysicalDevice::Release()
{
    LOG_SCOPE("Release Physical Device", /*hasNextSibling=*/false);
    {
        // Reset handles
        LOG_SCOPE("Reset Handles", /*hasNextSibling=*/true);
        {
            m_pPhysicalDevice.Reset();
            m_pSurface.Reset();
            LOG_SUCCESS("Handles reset (physical device + surface)");
        }
        LOG_SCOPE_END();

        // Clear cached data
        LOG_SCOPE("Clear Cached Data", /*hasNextSibling=*/false);
        {
            m_props      = {};
            m_memProps   = {};
            m_features2  = {};
            m_features11 = {};
            m_features12 = {};
            m_features13 = {};
            m_qfIndices  = {};
            m_ppDeviceExtProps.clear();
            m_ppEnabledDeviceExtensions.clear();
            m_ppAllDevices.clear();

            LOG_SUCCESS("All cached properties, features, and device lists cleared");
        }
        LOG_SCOPE_END();
    }

    LOG_SUCCESS("Physical device released");
}

VkPhysicalDevice FxPhysicalDevice::Get() const
{
    return m_pPhysicalDevice.Get();
}

VkInstance FxPhysicalDevice::Instance() const
{
    return m_pInstance ? m_pInstance->GetInstance() : VK_NULL_HANDLE;
}

VkSurfaceKHR FxPhysicalDevice::Surface() const
{
    return m_pSurface.Get();
}

const VkPhysicalDeviceProperties& FxPhysicalDevice::Properties() const
{
    return m_props;
}

const VkPhysicalDeviceMemoryProperties& FxPhysicalDevice::MemoryProperties() const
{
    return m_memProps;
}

const VkPhysicalDeviceFeatures2& FxPhysicalDevice::Features2() const
{
    return m_features2;
}

const VkPhysicalDeviceVulkan11Features& FxPhysicalDevice::Features11() const
{
    return m_features11;
}

const VkPhysicalDeviceVulkan12Features& FxPhysicalDevice::Features12() const
{
    return m_features12;
}

const VkPhysicalDeviceVulkan13Features& FxPhysicalDevice::Features13() const
{
    return m_features13;
}

const FX_QUEUE_FAMILY_INDEX_DESC & FxPhysicalDevice::Queues() const
{
    return m_qfIndices;
}

const std::vector<const char*>& FxPhysicalDevice::EnabledExtensions() const
{
    return m_ppEnabledDeviceExtensions;
}

const std::vector<VkExtensionProperties>& FxPhysicalDevice::AvailableExtensions() const
{
    return m_ppDeviceExtProps;
}

bool FxPhysicalDevice::HasExt(const std::vector<VkExtensionProperties>& exts, const char* name)
{
    for (const auto& e : exts)
    {
        if (std::strcmp(e.extensionName, name) == 0)
        {
            LOG_INFO("Extension '{}' found", name);
            return true;
        }
    }
    LOG_WARNING("Extension '{}' not found", name);
    return false;
}

FxPhysicalDevice::FxPhysicalDevice(FxPhysicalDevice&& other) noexcept
{
    *this = std::move(other);
}

FxPhysicalDevice& FxPhysicalDevice::operator=(FxPhysicalDevice&& other) noexcept
{
    if (this != &other)
    {
        Release();

        // Shallow / direct moves
        m_pInstance  = other.m_pInstance;
        m_policy    = other.m_policy;

        // Move RAII + containers
        m_pSurface                  = std::move(other.m_pSurface);
        m_pPhysicalDevice           = std::move(other.m_pPhysicalDevice);
        m_ppDeviceExtProps          = std::move(other.m_ppDeviceExtProps);
        m_ppEnabledDeviceExtensions = std::move(other.m_ppEnabledDeviceExtensions);
        m_ppAllDevices              = std::move(other.m_ppAllDevices);

        // Copy trivial cached structs
        m_props      = other.m_props;
        m_memProps   = other.m_memProps;
        m_features2  = other.m_features2;
        m_features11 = other.m_features11;
        m_features12 = other.m_features12;
        m_features13 = other.m_features13;
        m_qfIndices  = other.m_qfIndices;

        // Leave 'other' in a valid empty state
        other.m_pInstance = nullptr;
        other.m_pPhysicalDevice.Reset();
        other.m_props      = {};
        other.m_memProps   = {};
        other.m_features2  = {};
        other.m_features11 = {};
        other.m_features12 = {};
        other.m_features13 = {};
        other.m_qfIndices  = {};
        other.m_ppDeviceExtProps.clear();
        other.m_ppEnabledDeviceExtensions.clear();
        other.m_ppAllDevices.clear();
    }
    return *this;
}

bool FxPhysicalDevice::EnumeratePhysicalDevices()
{
    LOG_INFO("Enumerating Vulkan physical devices...");
    LOG_ADD_TAB();

    if (!m_pInstance)
    {
        LOG_ERROR("No FxInstance attached");
        LOG_REMOVE_TAB();
        return false;
    }

    const VkInstance inst = m_pInstance->GetInstance();
    uint32_t count = 0;

    VkResult vr = vkEnumeratePhysicalDevices(inst, &count, nullptr);
    if (vr != VK_SUCCESS)
    {
        LOG_ERROR("vkEnumeratePhysicalDevices(count=null) failed: VkResult={}", static_cast<int>(vr));
        LOG_REMOVE_TAB();
        return false;
    }

    if (count == 0)
    {
        LOG_ERROR("No Vulkan-capable physical devices found");
        LOG_REMOVE_TAB();
        return false;
    }

    std::vector<VkPhysicalDevice> tmp(count);
    vr = vkEnumeratePhysicalDevices(inst, &count, tmp.data());
    if (vr != VK_SUCCESS)
    {
        LOG_ERROR("vkEnumeratePhysicalDevices(handles) failed: VkResult={}", static_cast<int>(vr));
        LOG_REMOVE_TAB();
        return false;
    }

    m_ppAllDevices.assign(tmp.begin(), tmp.end());

    LOG_SUCCESS("Found {} physical device(s)", static_cast<int>(m_ppAllDevices.size()));

    // Log info for each device
    auto typeToStr = [](VkPhysicalDeviceType t) -> const char*
    {
        switch (t)
        {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete GPU";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual GPU";
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
            default:                                     return "Other";
        }
    };

    LOG_ADD_TAB();
    for (size_t i = 0; i < m_ppAllDevices.size(); ++i)
    {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(m_ppAllDevices[i], &props);
        LOG_INFO("[#{}] {} | {} | API {}.{}.{}",
                 static_cast<int>(i),
                 props.deviceName,
                 typeToStr(props.deviceType),
                 VK_VERSION_MAJOR(props.apiVersion),
                 VK_VERSION_MINOR(props.apiVersion),
                 VK_VERSION_PATCH(props.apiVersion));
    }
    LOG_REMOVE_TAB();

    LOG_REMOVE_TAB();
    return true;
}

int FxPhysicalDevice::PickBestDeviceIndex()
{
    LOG_INFO("Scoring physical devices...");
    LOG_ADD_TAB();

    if (m_ppAllDevices.empty())
    {
        THROW_EXCEPTION_FMT("No devices to score (did you call EnumeratePhysicalDevices?)");
    }

    struct Candidate { int idx; long long score; bool ok; };
    std::vector<Candidate> cands;
    cands.reserve(m_ppAllDevices.size());

    auto typeToStr = [](VkPhysicalDeviceType t) -> const char*
    {
        switch (t)
        {
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete GPU";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual GPU";
            case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
            default:                                     return "Other";
        }
    };

    for (int i = 0; i < static_cast<int>(m_ppAllDevices.size()); ++i)
    {
        const VkPhysicalDevice pd = m_ppAllDevices[static_cast<size_t>(i)];

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(pd, &props);

        LOG_INFO("Evaluating [#{}] {} ({}) API {}.{}.{}",
                 i, props.deviceName, typeToStr(props.deviceType),
                 VK_VERSION_MAJOR(props.apiVersion),
                 VK_VERSION_MINOR(props.apiVersion),
                 VK_VERSION_PATCH(props.apiVersion));
        LOG_ADD_TAB();

        // 1) Device type preference
        const bool typeOk = std::find(m_policy.PreferredTypes.begin(),
                                      m_policy.PreferredTypes.end(),
                                      props.deviceType) != m_policy.PreferredTypes.end();
        if (!typeOk)
        {
            LOG_WARNING("Rejected: device type not in preferred list");
            LOG_REMOVE_TAB();
            cands.push_back({ i, std::numeric_limits<long long>::min(), false });
            continue;
        }

        // 2) Extensions
        std::vector<VkExtensionProperties> exts;
        if (!EnumerateDeviceExtensions(pd, exts))
        {
            THROW_EXCEPTION_FMT("Failed to enumerate device extensions for [{}]", props.deviceName);
        }

        if (m_policy.RequireSwapChain && !HasExt(exts, "VK_KHR_swapchain"))
        {
            LOG_WARNING("Rejected: missing VK_KHR_swapchain (RequireSwapChain=true)");
            LOG_REMOVE_TAB();
            cands.push_back({ i, std::numeric_limits<long long>::min(), false });
            continue;
        }

        bool missingReqExt = false;
        for (const char* req : m_policy.RequiredExtensions)
        {
            if (!HasExt(exts, req))
            {
                LOG_WARNING("Rejected: missing required extension '{}'", req);
                missingReqExt = true;
                break;
            }
        }
        if (missingReqExt)
        {
            LOG_REMOVE_TAB();
            cands.push_back({ i, std::numeric_limits<long long>::min(), false });
            continue;
        }

        // 3) Queue families
        FX_QUEUE_FAMILY_INDEX_DESC qfi{};
        if (!ProbeQueueFamilies(pd, qfi))
        {
            LOG_WARNING("Rejected: no suitable queue families found");
            LOG_REMOVE_TAB();
            cands.push_back({ i, std::numeric_limits<long long>::min(), false });
            continue;
        }
        if (!qfi.IsValid(m_policy.RequireSwapChain))
        {
            LOG_WARNING("Rejected: incomplete queues (graphics/present)");
            LOG_REMOVE_TAB();
            cands.push_back({ i, std::numeric_limits<long long>::min(), false });
            continue;
        }

        // 4) Score
        long long score = 0;
        // Type rank
        {
            auto it = std::ranges::find(m_policy.PreferredTypes, props.deviceType);
            const int rank = (it == m_policy.PreferredTypes.end())
                             ? 100
                             : static_cast<int>(std::distance(m_policy.PreferredTypes.begin(), it));
            score += (1000 - rank * 100);
        }
        // Max 2D image dimension
        score += static_cast<long long>(m_policy.WeightMaxImage2D) * props.limits.maxImageDimension2D;

        // VRAM in MB
        VkPhysicalDeviceMemoryProperties mem{};
        vkGetPhysicalDeviceMemoryProperties(pd, &mem);
        VkDeviceSize bestHeap = 0;
        for (uint32_t h = 0; h < mem.memoryHeapCount; ++h)
            if (mem.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                bestHeap = std::max(bestHeap, mem.memoryHeaps[h].size);
        const long long vramMB = static_cast<long long>(bestHeap / (1024ull * 1024ull));
        score += static_cast<long long>(m_policy.WeightVRam) * vramMB;

        LOG_SUCCESS("Accepted: Queues G={}, C={}, T={}, P={} | Score={} (VRAM~{} MB, MaxImage2D={})",
                    qfi.Graphics, qfi.Compute, qfi.Transfer, qfi.Present,
                    score, vramMB, props.limits.maxImageDimension2D);

        LOG_REMOVE_TAB();
        cands.push_back({ i, score, true });
    }

    // Pick best
    int bestIdx = -1;
    long long bestScore = std::numeric_limits<long long>::min();
    for (const auto& c : cands)
    {
        if (!c.ok) continue;
        if (c.score > bestScore)
        {
            bestScore = c.score;
            bestIdx = c.idx;
        }
    }

    if (bestIdx < 0) THROW_EXCEPTION_FMT("No suitable physical device found after scoring");

    LOG_SUCCESS("Selected device index {}", bestIdx);
    LOG_REMOVE_TAB();
    return bestIdx;
}

void FxPhysicalDevice::CacheDeviceBasics()
{
    if (!m_pPhysicalDevice.IsValid())
    {
        THROW_EXCEPTION_FMT("CacheDeviceBasics called without a valid physical device");
    }

    const VkPhysicalDevice pd = m_pPhysicalDevice.Get();

    // --- Basic properties & memory ---
    vkGetPhysicalDeviceProperties(pd, &m_props);
    vkGetPhysicalDeviceMemoryProperties(pd, &m_memProps);

    // --- Features chain (Vulkan 1.1, 1.2, 1.3) ---
    m_features13 = {};
    m_features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

    m_features12 = {};
    m_features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    m_features12.pNext = &m_features13;

    m_features11 = {};
    m_features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    m_features11.pNext = &m_features12;

    m_features2 = {};
    m_features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    m_features2.pNext = &m_features11;

    vkGetPhysicalDeviceFeatures2(pd, &m_features2);

    // --- Available extensions ---
    m_ppDeviceExtProps.clear();
    if (!EnumerateDeviceExtensions(pd, m_ppDeviceExtProps))
    {
        THROW_EXCEPTION_FMT("Failed to enumerate extensions for device '{}'", m_props.deviceName);
    }

    LOG_INFO("Cached properties for physical device '{}'", m_props.deviceName);
}

bool FxPhysicalDevice::ProbeQueueFamilies(VkPhysicalDevice pd, FX_QUEUE_FAMILY_INDEX_DESC& out) const
{
    LOG_PRINT("Probing queue families...");
    LOG_ADD_TAB();

    out = {}; // reset

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, nullptr);
    if (count == 0)
    {
        LOG_WARNING("No queue families reported by the device");
        LOG_REMOVE_TAB();
        return false;
    }

    std::vector<VkQueueFamilyProperties> qf(count);
    vkGetPhysicalDeviceQueueFamilyProperties(pd, &count, qf.data());

    int graphics = -1;
    int compute  = -1;
    int transfer = -1;
    int present  = -1;

    const VkSurfaceKHR surf = m_pSurface.Get();

    // Pass 1: find graphics and (optionally) present
    for (int i = 0; i < static_cast<int>(qf.size()); ++i)
    {
        const auto& info = qf[static_cast<size_t>(i)];

        if ((info.queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphics < 0)
            graphics = i;

        if (surf != VK_NULL_HANDLE)
        {
            VkBool32 sup = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(pd, static_cast<uint32_t>(i), surf, &sup);
            if (sup && present < 0)
                present = i;
        }
    }

    // Pass 2: compute (prefer dedicated if requested)
    if (m_policy.PreferDedicatedCompute)
    {
        for (int i = 0; i < static_cast<int>(qf.size()); ++i)
        {
            const auto& info = qf[static_cast<size_t>(i)];
            if ((info.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                !(info.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            { compute = i; break; }
        }
    }
    if (compute < 0)
    {
        for (int i = 0; i < static_cast<int>(qf.size()); ++i)
        {
            if (qf[static_cast<size_t>(i)].queueFlags & VK_QUEUE_COMPUTE_BIT)
            { compute = i; break; }
        }
    }

    // Pass 3: transfer (prefer dedicated if requested)
    if (m_policy.PreferDedicatedTransfer)
    {
        for (int i = 0; i < static_cast<int>(qf.size()); ++i)
        {
            const auto& info = qf[static_cast<size_t>(i)];
            if ((info.queueFlags  & VK_QUEUE_TRANSFER_BIT) &&
                !(info.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                !(info.queueFlags & VK_QUEUE_COMPUTE_BIT))
            { transfer = i; break; }
        }
    }
    if (transfer < 0)
    {
        for (int i = 0; i < static_cast<int>(qf.size()); ++i)
        {
            if (qf[static_cast<size_t>(i)].queueFlags & VK_QUEUE_TRANSFER_BIT)
            { transfer = i; break; }
        }
    }

    // Fallbacks: collapse to graphics where needed
    out.Graphics = graphics;
    out.Compute  = (compute  >= 0) ? compute  : graphics;
    out.Transfer = (transfer >= 0) ? transfer : graphics;
    out.Present  = (present  >= 0) ? present  : graphics;

    const bool ok = out.IsValid(m_policy.RequireSwapChain);

    LOG_INFO("Queues -> G={}, C={}, T={}, P={} (requirePresent={})",
             out.Graphics, out.Compute, out.Transfer, out.Present,
             m_policy.RequireSwapChain ? "true" : "false");

    if (!ok)
        LOG_WARNING("Queue families incomplete for current requirements");

    LOG_REMOVE_TAB();
    return ok;
}

bool FxPhysicalDevice::FindQueueFamilies()
{
    LOG_INFO("Finding queue families on selected physical device...");
    LOG_ADD_TAB();

    if (!m_pPhysicalDevice.IsValid())
    {
        LOG_ERROR("FindQueueFamilies called before selecting a physical device");
        LOG_REMOVE_TAB();
        THROW_EXCEPTION_FMT("FindQueueFamilies: no physical device selected");
    }

    const VkPhysicalDevice pd = m_pPhysicalDevice.Get();

    FX_QUEUE_FAMILY_INDEX_DESC qfi{};
    const bool ok = ProbeQueueFamilies(pd, qfi);

    if (!ok || !qfi.IsValid(m_policy.RequireSwapChain))
    {
        LOG_ERROR("Required queue families not satisfied (RequireSwapChain={})",
                  m_policy.RequireSwapChain ? "true" : "false");
        LOG_REMOVE_TAB();
        THROW_EXCEPTION_FMT("Selected physical device does not provide required queue families");
    }

    m_qfIndices = qfi;

    LOG_SUCCESS("Queues -> G={}, C={}, T={}, P={}",
                m_qfIndices.Graphics, m_qfIndices.Compute,
                m_qfIndices.Transfer, m_qfIndices.Present);
    LOG_REMOVE_TAB();
    return true;
}

bool FxPhysicalDevice::EnumerateDeviceExtensions(VkPhysicalDevice pd, std::vector<VkExtensionProperties>& out)
{
    LOG_PRINT("Enumerating device extensions...");
    LOG_ADD_TAB();

    out.clear();

    uint32_t count = 0;
    VkResult vr = vkEnumerateDeviceExtensionProperties(pd, /*pLayerName*/nullptr, &count, nullptr);
    if (vr != VK_SUCCESS)
    {
        LOG_ERROR("vkEnumerateDeviceExtensionProperties(count=null) failed: VkResult={}", static_cast<int>(vr));
        LOG_REMOVE_TAB();
        return false;
    }

    if (count == 0)
    {
        LOG_WARNING("Device reports 0 extensions");
        LOG_REMOVE_TAB();
        return true; // success (empty list)
    }

    out.resize(count);
    vr = vkEnumerateDeviceExtensionProperties(pd, nullptr, &count, out.data());
    if (vr != VK_SUCCESS)
    {
        LOG_ERROR("vkEnumerateDeviceExtensionProperties(handles) failed: VkResult={}", static_cast<int>(vr));
        out.clear();
        LOG_REMOVE_TAB();
        return false;
    }

    LOG_INFO("Found {} device extension(s)", static_cast<int>(out.size()));
    LOG_REMOVE_TAB();
    return true;
}

bool FxPhysicalDevice::ResolveExtensions()
{
    LOG_PRINT("Resolving required/optional device extensions...");
    LOG_ADD_TAB();

    // m_deviceExtProps must already be cached by CacheDeviceBasics()
    m_ppEnabledDeviceExtensions.clear();

    // Required extensions (critical if missing)
    for (const char* req : m_policy.RequiredExtensions)
    {
        if (!HasExt(m_ppDeviceExtProps, req))
        {
            LOG_ERROR("Missing required device extension '{}'", req);
            LOG_REMOVE_TAB();
            THROW_EXCEPTION_FMT("FxPhysicalDevice::ResolveExtensions: missing required extension '{}'", req);
        }
        m_ppEnabledDeviceExtensions.push_back(req);
    }

    // silently skip if absent
    for (const char* opt : m_policy.OptionalExtensions)
    {
        if (HasExt(m_ppDeviceExtProps, opt))
        {
            m_ppEnabledDeviceExtensions.push_back(opt);
            LOG_INFO("Enabled optional extension '{}'", opt);
        }
    }

    LOG_SUCCESS("Enabled {} device extension(s)", static_cast<int>(m_ppEnabledDeviceExtensions.size()));
    LOG_REMOVE_TAB();
    return true;
}

bool FxPhysicalDevice::ValidateRequiredCoreFeatures()
{
    LOG_PRINT("Validating required core features for '{}'", m_props.deviceName);
    LOG_ADD_TAB();

    const VkPhysicalDeviceFeatures& f = m_features2.features;

    if (m_policy.RequiredCoreFeatures.SampleAnisotropy && !f.samplerAnisotropy)
    {
        LOG_ERROR("Required feature 'samplerAnisotropy' not supported");
        LOG_REMOVE_TAB();
        THROW_EXCEPTION_FMT("Device '{}' lacks required feature: samplerAnisotropy", m_props.deviceName);
    }

    if (m_policy.RequiredCoreFeatures.GeometryShader && !f.geometryShader)
    {
        LOG_ERROR("Required feature 'geometryShader' not supported");
        LOG_REMOVE_TAB();
        THROW_EXCEPTION_FMT("Device '{}' lacks required feature: geometryShader", m_props.deviceName);
    }

    if (m_policy.RequiredCoreFeatures.FillModeNonSolid && !f.fillModeNonSolid)
    {
        LOG_ERROR("Required feature 'fillModeNonSolid' not supported");
        LOG_REMOVE_TAB();
        THROW_EXCEPTION_FMT("Device '{}' lacks required feature: fillModeNonSolid", m_props.deviceName);
    }

    LOG_SUCCESS("All required core features are supported");
    LOG_REMOVE_TAB();
    return true;
}
