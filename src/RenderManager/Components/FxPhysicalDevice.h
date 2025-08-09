//
// Created by niffo on 8/9/2025.
//

#ifndef FXPHYSICALDEVICE_H
#define FXPHYSICALDEVICE_H

#include "Common/DefineVulkan.h"
#include "Interface/IGfxObject.h"
#include "Common/FxMemory.h"

#include <vector>

class FxInstance;

typedef struct FX_QUEUE_FAMILY_INDEX_DESC
{
    int Graphics{ -1 };
    int Compute { -1 };
    int Transfer{ -1 };
    int Present { -1 };

    FORCELINE bool IsValid(const bool requirePresent) const
    {
        if (const bool base = Graphics >= 0; !base) return false;
        if (requirePresent && Present < 0) return false;
        return true;
    }
} FX_QUEUE_FAMILY_INDEX_DESC;

/*** Physical Device Selection Policy */
typedef struct FX_PD_SELECTION_POLICY
{
    bool RequireSwapChain{ true };

    std::vector<VkPhysicalDeviceType> PreferredTypes =
    {
        VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
        VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
        VK_PHYSICAL_DEVICE_TYPE_CPU,
        VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
        VK_PHYSICAL_DEVICE_TYPE_OTHER
    };

    //~ Device Extension
    std::vector<const char*> RequiredExtensions = { "VK_KHR_swapchain" };
    std::vector<const char*> OptionalExtensions;

    //~ Core Feature requirements
    struct
    {
        VkBool32 SampleAnisotropy{ VK_FALSE };
        VkBool32 GeometryShader  { VK_FALSE };
        VkBool32 FillModeNonSolid{ VK_FALSE };
    } RequiredCoreFeatures{};

    //~ Queue Preference
    bool PreferDedicatedCompute { true };
    bool PreferDedicatedTransfer{ true };

    //~ scoring knobs
    int WeightMaxImage2D = 1;
    int WeightVRam       = 1;

} FX_PD_SELECTION_POLICY_DESC;

/** Vulkan Physical Device */
class FxPhysicalDevice final: public IGfxObject
{
public:
    FxPhysicalDevice() = default;
    ~FxPhysicalDevice() override;

    //~ Configure selection policy
    void Describe(_fox_In_ const FX_PD_SELECTION_POLICY_DESC& desc);

    void AttachInstance(
        _fox_In_ const FxInstance& instance,
        _fox_In_ VkSurfaceKHR surface = VK_NULL_HANDLE
    );
    void DetachInstance();
    void SetSurface(_fox_In_ VkSurfaceKHR surface);

    //~ GFX Object Impl
    bool Init() override;
    void Release() override;

    // Getters
    _fox_Return_enforce VkPhysicalDevice Get     () const;
    _fox_Return_enforce VkInstance       Instance() const;
    _fox_Return_enforce VkSurfaceKHR     Surface () const;

    _fox_Return_enforce const VkPhysicalDeviceProperties&       Properties      () const;
    _fox_Return_enforce const VkPhysicalDeviceMemoryProperties& MemoryProperties() const;

    _fox_Return_enforce const VkPhysicalDeviceFeatures2&        Features2 ()  const;
    _fox_Return_enforce const VkPhysicalDeviceVulkan11Features& Features11()  const;
    _fox_Return_enforce const VkPhysicalDeviceVulkan12Features& Features12()  const;
    _fox_Return_enforce const VkPhysicalDeviceVulkan13Features& Features13()  const;

    _fox_Return_enforce const FX_QUEUE_FAMILY_INDEX_DESC&         Queues             () const;
    _fox_Return_enforce const std::vector<const char*>&           EnabledExtensions  () const;
    _fox_Return_enforce const std::vector<VkExtensionProperties>& AvailableExtensions() const;

    bool HasExtension(_fox_In_ const char* name) const;

    FxPhysicalDevice(const FxPhysicalDevice&)            = delete;
    FxPhysicalDevice& operator=(const FxPhysicalDevice&) = delete;

    FxPhysicalDevice(FxPhysicalDevice&&) noexcept;
    FxPhysicalDevice& operator=(FxPhysicalDevice&&) noexcept;

private:
    // Enumeration & selection
    bool EnumeratePhysicalDevices();
    int  PickBestDeviceIndex();

    // Cache queried data for the selected device
    void CacheDeviceBasics();

    // Queue families
    bool ProbeQueueFamilies(_fox_In_ VkPhysicalDevice pd,
                            _fox_In_ FX_QUEUE_FAMILY_INDEX_DESC& out) const;
    bool FindQueueFamilies();

    static bool EnumerateDeviceExtensions(
        _fox_In_ VkPhysicalDevice pd,
        _fox_In_ std::vector<VkExtensionProperties>& out
    );

    static bool HasExt(
        _fox_In_ const std::vector<VkExtensionProperties>& exts,
        _fox_In_ const char* name
    );
    bool ResolveExtensions();
    bool ValidateRequiredCoreFeatures();

private:
    const FxInstance*    m_pInstance { nullptr };

    FxPtr<VkSurfaceKHR>     m_pSurface;
    FX_PD_SELECTION_POLICY  m_policy{};

    // Picked device & cached data
    FxPtr<VkPhysicalDevice>            m_pPhysicalDevice;
    VkPhysicalDeviceProperties         m_props{};
    VkPhysicalDeviceMemoryProperties   m_memProps{};

    // Features2 chain
    VkPhysicalDeviceFeatures2          m_features2{};
    VkPhysicalDeviceVulkan11Features   m_features11{};
    VkPhysicalDeviceVulkan12Features   m_features12{};
    VkPhysicalDeviceVulkan13Features   m_features13{};

    // Queues
    FX_QUEUE_FAMILY_INDEX_DESC m_qfIndices{};

    // Extensions
    std::vector<VkExtensionProperties> m_ppDeviceExtProps;
    std::vector<const char*>           m_ppEnabledDeviceExtensions;

    // Enumeration temps
    std::vector<VkPhysicalDevice> m_ppAllDevices;
};

#endif //FXPHYSICALDEVICE_H
