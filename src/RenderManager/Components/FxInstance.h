//
// Created by niffo on 8/4/2025.
//

#ifndef FXINSTANCE_H
#define FXINSTANCE_H
#include "Common/FxMemory.h"
#include "Interface/IGfxObject.h"

//~ Configure instance desc
typedef struct FOX_INSTANCE_CREATE_DESC
{
    std::string AppName       = "FoxRenderPlayground";
    uint32_t    AppVersion    = VK_MAKE_VERSION(1, 0, 0);
    std::string EngineName    = "FxRenderEngine";
    uint32_t    EngineVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t    ApiVersion    = VK_API_VERSION_1_4;

    std::vector<const char*> EnabledExtensionNames;
    std::vector<const char*> EnabledLayerNames;

#if defined(_DEBUG) || defined(DEBUG)
    bool EnableDebug = true;
#else
    bool EnableDebug = false;
#endif

    VkAllocationCallbacks* pAllocator = nullptr;
} FOX_INSTANCE_CREATE_DESC;

/*** Wrapper for Vulkan Instance */
class FxInstance final: public IGfxObject
{
public:
     FxInstance()          = default;
    ~FxInstance() override = default;

    bool Init   () override;
    void Release() override;

    void Describe(_fox_In_ const FOX_INSTANCE_CREATE_DESC& desc);

    _fox_Return_enforce
    VkInstance GetInstance() const { return m_pInstance.Get(); }

    bool SupportsExtension(_fox_In_ const char* extensinName) const;
    bool SupportsLayer    (_fox_In_ const char* layerName)    const;

    //~ Restrict Copy
    FxInstance(const FxInstance&)            = delete;
    FxInstance& operator=(const FxInstance&) = delete;

    //~ Allow Move
    FxInstance(FxInstance&&)            = default;
    FxInstance& operator=(FxInstance&&) = default;

private:
    void SetupDebugMessenger();
    void DestroyDebugMessenger();

    void FillAppInfo   (_fox_In_ const FOX_INSTANCE_CREATE_DESC& desc);
    void PickExtensions(_fox_In_ const FOX_INSTANCE_CREATE_DESC& desc);
    void PickLayers    (_fox_In_ const FOX_INSTANCE_CREATE_DESC& desc);

    void FillInstanceCreateInfo();

private:
    FxPtr<VkInstance>        m_pInstance;
    VkApplicationInfo        m_infoVkApp;
    VkInstanceCreateInfo     m_infoVkInstance;
    std::vector<VkExtensionProperties> m_ppEnabledExtensions;
    std::vector<const char*> m_ppEnabledLayerNames;
    std::vector<VkLayerProperties> m_ppEnabledLayers;
    std::vector<const char*> m_ppEnabledExtensionNames;
    FOX_INSTANCE_CREATE_DESC m_descInstance;


#if defined(_DEBUG) || defined(DEBUG)
    FxPtr<VkDebugUtilsMessengerEXT> m_pDebugMessenger;
#endif

    VkAllocationCallbacks* m_pAllocator{ nullptr };
};

#endif //FXINSTANCE_H
