//
// Created by niffo on 7/22/2025.
//

#include "DependencyResolver.h"

#include <algorithm>
#include <cassert>

#include "ExceptionHandler/IException.h"
#include "Logger/Logger.h"

void DependencyResolver::Register(ISystem *pSystem)
{
    if (pSystem == nullptr) return;

    for (const auto& pHave: m_ppSystems)
        if (pSystem->GetSystemName() == pHave->GetSystemName()) return;

    m_ppSystems.emplace_back(pSystem);
}

void DependencyResolver::Unregister(ISystem *pSystem)
{
    if (pSystem == nullptr) return;

    std::erase_if(m_ppSystems, [pSystem](ISystem* s)
    {
        return s->GetSystemName() == pSystem->GetSystemName();
    });
}

template<typename ... Args>
bool DependencyResolver::InitializeSystems(Args &...args)
{
    LOG_WARNING("Attempting to initialize systems...");
    SortTopologically();

    for (ISystem* system: m_ppSystems)
    {
        if (!system->OnInit(args...))
        {
            LOG_ERROR("[DependencyResolver] Failed to {}", system->GetSystemName());
            return false;
        }
        LOG_INFO("[DependencyResolver] Initialise {}...", system->GetSystemName());
    }
    LOG_SUCCESS("[DependencyResolver] Initialisation done.");
    return true;
}

template<typename ... Args>
void DependencyResolver::UpdateStartSystems(Args &...args) const
{
    for (ISystem* system: m_ppSystems)
        system->OnUpdateStart(args...);

}

template<typename ... Args>
void DependencyResolver::UpdateEndSystems(Args &...args) const
{
    for (ISystem* system: m_ppSystems)
        system->OnUpdateEnd(args...);
}

template<typename ... Args>
void DependencyResolver::ReleaseSystems(Args &...args)
{
    for (ISystem* system: m_ppSystems)
        system->OnRelease(args...);
}

template<typename ... Args>
void DependencyResolver::AddDependency(ISystem *pSystem, Args &...args)
{
    (m_ppSystemsDependencies[pSystem].emplace_back(args), ...);
}

void DependencyResolver::Clean()
{
    m_ppSystemsDependencies.clear();
    m_ppSystems.clear();
}

bool DependencyResolver::SortTopologically()
{
    std::unordered_set<ISystem*> visited;
    std::unordered_set<ISystem*> recursionStack;
    std::vector<ISystem*> sorted;

    for (ISystem* system : m_ppSystems)
    {
        if (!visited.contains(system))
        {
            DFS(system, visited, recursionStack, sorted);
            if (sorted.empty()) return false; // cycle detected
        }
    }

    m_ppSystems.clear();
    m_ppSystems = std::move(sorted);
    return true;
}

void DependencyResolver::DFS(
    ISystem *node,
    std::unordered_set<ISystem*> &visited,
    std::unordered_set<ISystem*> &recursionStack,
    std::vector<ISystem*> &sorted)
{
    if (recursionStack.contains(node))
    {
        THROW_EXCEPTION_MSG(
            "Cycle Detected with {}",
            node->GetSystemName()
        );
        sorted.clear(); // cycle detected!
        return;
    }
    if (visited.contains(node)) return;

    recursionStack.insert(node);
    visited.insert(node);

    if (m_ppSystemsDependencies.contains(node))
    {
        for (ISystem* dep: m_ppSystemsDependencies[node])
        {
            DFS(dep, visited, recursionStack, sorted);
            if (sorted.empty()) return;
        }
    }
    recursionStack.erase(node);
    sorted.push_back(node);
}
