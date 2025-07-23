//
// Created by niffo on 7/22/2025.
//

#include "DependencyResolver.h"
#include "ExceptionHandler/IException.h"
#include "Logger/Logger.h"

#include <algorithm>
#include <cassert>

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

    std::erase_if(m_ppSystems, [pSystem](const ISystem* s)
    {
        return s->GetSystemName() == pSystem->GetSystemName();
    });
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
#if defined(_DEBUG) || defined(DEBUG)
    if (node == nullptr) __debugbreak();
#else
    if (node == nullptr) THROW_EXCEPTION_MSG("Null node in DFS traversal.");
#endif

    if (recursionStack.contains(node))
    {
        const FString msg =  "Cycle Detected with" + node->GetSystemName();
        THROW_EXCEPTION_MSG(msg.c_str());
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
