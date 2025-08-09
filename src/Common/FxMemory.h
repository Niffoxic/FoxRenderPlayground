//
// Created by niffo on 8/4/2025.
//

#ifndef FXMEMORY_H
#define FXMEMORY_H

#define NOMINMAX

#include "Common/Core.h"
#include <functional>

template<typename T>
class FxPtr
{
    using ResetCallback = std::function<void(T)>;

public:
    FxPtr() = default;

    FxPtr(_fox_In_ T handle, _fox_In_ ResetCallback destructor)
        : m_handle(handle), m_destructor(std::move(destructor)) {}

    ~FxPtr() { Reset(); }

    // Destroy current, then optionally adopt a new handle + destructor
    void Reset(_fox_In_ T newHandle = T{}, _fox_In_ ResetCallback destructor = nullptr)
    {
        if (m_handle != T{} && m_destructor)
            m_destructor(m_handle);

        m_handle     = newHandle;
        m_destructor = std::move(destructor);
    }

    // Adopt without destroying current (use with care)
    void Disarm() { m_handle = T{}; m_destructor = nullptr; }

    // Convenience for vkCreate* out-params
    T* Put(_fox_In_ ResetCallback destructor)
    {
        Reset();
        m_destructor = std::move(destructor);
        return &m_handle;              // API will fill this
    }

    // Access
    T Get() const { return m_handle; }
    explicit operator T() const { return m_handle; }

    // no copy
    FxPtr(const FxPtr&)            = delete;
    FxPtr& operator=(const FxPtr&) = delete;

    // move only
    FxPtr(FxPtr&& other) noexcept
        : m_handle(other.m_handle), m_destructor(std::move(other.m_destructor))
    {
        other.m_handle = T{};
    }

    FxPtr& operator=(FxPtr&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_handle     = other.m_handle;
            m_destructor = std::move(other.m_destructor);
            other.m_handle = T{};
        }
        return *this;
    }

    _fox_Return_enforce bool IsValid()        const { return m_handle != T{}; }
    _fox_Return_enforce bool IsDestructible() const { return static_cast<bool>(m_destructor); }

private:
    T             m_handle{};
    ResetCallback m_destructor{};
};


#endif //FXMEMORY_H
