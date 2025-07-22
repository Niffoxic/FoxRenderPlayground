//
// Created by niffo on 7/22/2025.
//

#ifndef DEPENDENCYRESOLVER_H
#define DEPENDENCYRESOLVER_H

#include "Common/Core.h"
#include <functional>
#include <unordered_set>

#include "Interface/ISystem.h"

class DependencyResolver
{
public:
    DependencyResolver() = default;
    ~DependencyResolver() = default;

    DependencyResolver(const DependencyResolver&) = delete;
    DependencyResolver(DependencyResolver&&)      = delete;

    DependencyResolver& operator=(const DependencyResolver&) = delete;
    DependencyResolver& operator=(DependencyResolver&&)      = delete;

    void Register(ISystem* pSystem);
    void Unregister(ISystem* pSystem);

    template<typename... Args>
    bool InitializeSystems(Args&... args);

    template<typename... Args>
    void UpdateStartSystems(Args&... args) const;

    template<typename... Args>
    void UpdateEndSystems(Args&... args) const;

    template<typename... Args>
    void ReleaseSystems(Args&... args);

    template<typename... Args>
    void AddDependency(ISystem* pSystem, Args&... args);

    void Clean();

private:
    bool SortTopologically();

    void DFS(
        ISystem* node,
        std::unordered_set<ISystem*>& visited,
        std::unordered_set<ISystem*>& recursionStack,
        std::vector<ISystem*>& sorted);

private:
    std::vector<ISystem*> m_ppSystems;
    std::unordered_map<ISystem*, std::vector<ISystem*>> m_ppSystemsDependencies;
};

#endif //DEPENDENCYRESOLVER_H
