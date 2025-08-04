//
// Created by niffo on 7/22/2025.
//

#ifndef DEPENDENCYRESOLVER_H
#define DEPENDENCYRESOLVER_H

#include "Common/Core.h"
#include "Interface/ISystem.h"
#include "Logger/Logger.h"

#include <functional>
#include <unordered_set>


class DependencyResolver
{
    using DependencyRegister = std::unordered_map<ISystem*, std::vector<ISystem*>>;
public:
     DependencyResolver() = default;
    ~DependencyResolver() = default;

    DependencyResolver(const DependencyResolver&) = delete;
    DependencyResolver(DependencyResolver&&)      = delete;

    DependencyResolver& operator=(const DependencyResolver&) = delete;
    DependencyResolver& operator=(DependencyResolver&&)      = delete;

    void Register(ISystem *pSystem);

    template<typename... Args>
    void Register(_fox_In_ Args*... args);

    void Unregister(_fox_In_ ISystem* pSystem)
    _fox_Pre_satisfies_(pSystem != nullptr);

    template<typename... Args>
    void Unregister(_fox_In_ Args*... args);

    template<typename... Args> _fox_Return_enforce
    bool InitializeSystems(Args&... args)
    _fox_Success_(return == true);

    template<typename... Args>
    void UpdateStartSystems(Args... args) const;

    template<typename... Args>
    void UpdateEndSystems(Args... args) const;

    template<typename... Args>
    void ReleaseSystems(Args&... args);

    template<typename... Args>
    void AddDependency(_fox_In_ ISystem* pSystem, Args*... args)
    _fox_Pre_satisfies_(pSystem != nullptr);

    void Clean();

private:
    bool SortTopologically();

    void DFS(
        _fox_In_ ISystem* node,
        _fox_Inout_ std::unordered_set<ISystem*>& visited,
        _fox_Inout_ std::unordered_set<ISystem*>& recursionStack,
        _fox_Inout_ std::vector<ISystem*>& sorted
    );

private:
    std::vector<ISystem*> m_ppSystems;
    DependencyRegister    m_ppSystemsDependencies;
};

template<typename ... Args>
inline void DependencyResolver::Register(Args *...args)
{
    static_assert((std::is_base_of_v<ISystem, Args> && ...), "All args must be derived from ISystem");
    (Register(static_cast<ISystem*>(args)), ...);
}


template<typename ... Args>
inline void DependencyResolver::Unregister(Args *...args)
{
    static_assert((std::is_base_of_v<ISystem, Args> && ...), "All args must be derived from ISystem");
    (Unregister(static_cast<ISystem*>(args)), ...);
}

template<typename ... Args>
inline bool DependencyResolver::InitializeSystems(Args &...args)
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
inline void DependencyResolver::UpdateStartSystems(Args...args) const
{
    for (ISystem* system: m_ppSystems) system->OnUpdateStart(args...);
}

template<typename ... Args>
inline void DependencyResolver::UpdateEndSystems(Args...args) const
{
    for (ISystem* system: m_ppSystems) system->OnUpdateEnd(args...);
}

template<typename ... Args>
inline void DependencyResolver::ReleaseSystems(Args &...args)
{
    for (ISystem* system: m_ppSystems) system->OnRelease(args...);
}

template<typename ... Args>
inline void DependencyResolver::AddDependency(ISystem *pSystem, Args*...args)
{
    (m_ppSystemsDependencies[pSystem].emplace_back(args), ...);
}

#endif //DEPENDENCYRESOLVER_H
