//
// Created by niffo on 7/16/2025.
//

#include "RenderManager.h"

RenderManager::RenderManager(WindowsManager *winManager)
 : m_pWinManager(winManager)
{}

RenderManager::~RenderManager()
{
    OnRelease();
}

bool RenderManager::OnInit()
{
    // Create objects
    m_pInstance       = std::make_unique<FxInstance>();
    m_pPhysicalDevice = std::make_unique<FxPhysicalDevice>();

    // Vulkan Instance
    LOG_SCOPE("Vulkan Instance", /*hasNextSibling=*/true);
    {
        if (!m_pInstance->Init())
        {
            LOG_ERROR("Failed to create Vulkan instance");
            LOG_SCOPE_END();
            return false;
        }
        LOG_SUCCESS("Instance created");
    }
    LOG_SCOPE_END();

    // Physical Device
    LOG_SCOPE("Physical Device", /*hasNextSibling=*/false);
    {
        FX_PD_SELECTION_POLICY_DESC pol{};
        pol.RequireSwapChain = false;
        m_pPhysicalDevice->Describe(pol);
        m_pPhysicalDevice->AttachInstance(*m_pInstance /*, surface */);

        if (!m_pPhysicalDevice->Init())
        {
            LOG_ERROR("Failed to initialize physical device");
            LOG_SCOPE_END();
            return false;
        }

        LOG_SUCCESS("Selected GPU: {}", m_pPhysicalDevice->Properties().deviceName);
        LOG_INFO("Queues -> G={}, C={}, T={}, P={}",
                 m_pPhysicalDevice->Queues().Graphics,
                 m_pPhysicalDevice->Queues().Compute,
                 m_pPhysicalDevice->Queues().Transfer,
                 m_pPhysicalDevice->Queues().Present);
    }
    LOG_SCOPE_END();

    return true;
}

void RenderManager::OnUpdateStart(const float deltaTime)
{

}

void RenderManager::OnUpdateEnd()
{

}

void RenderManager::OnRelease()
{
    if (m_pPhysicalDevice) m_pPhysicalDevice->Release();
    if (m_pInstance)       m_pInstance->Release();

    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}
