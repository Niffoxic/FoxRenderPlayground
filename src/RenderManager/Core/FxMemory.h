//
// Created by niffo on 8/4/2025.
//

#ifndef FXMEMORY_H
#define FXMEMORY_H

#define NOMINMAX

#include "Common/Core.h"
#include <functional>
#include <vulkan/vulkan.h>

template<typename T>
class FxPtr
{
    using ResetCallback = std::function<void(T)>;
public:
    FxPtr() = default;
    FxPtr(
        _fox_In_ T handle,
        _fox_In_ ResetCallback destructor
    )
    : m_handle(handle), m_destructor(destructor)
    {}

    ~FxPtr(){ Reset(); }

    void Reset(
        _fox_In_ T newHandle = VK_NULL_HANDLE,
        _fox_In_ ResetCallback destructor = nullptr
    )
    {
        if (m_handle != VK_NULL_HANDLE && m_destructor != nullptr) m_destructor = destructor;

        m_handle = newHandle;
        m_destructor = destructor;
    }

    T Get() const { return m_handle; }
    explicit operator T() const { return m_handle; }

    // no copy
    FxPtr(const FxPtr&)            = delete;
    FxPtr& operator=(const FxPtr&) = delete;

    // move only
    FxPtr(FxPtr&& other) noexcept
    : m_handle(other.m_handle), m_destructor(other.m_destructor)
    {
        other.m_handle = VK_NULL_HANDLE;
    }

    FxPtr& operator=(FxPtr&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_handle = other.m_handle;
            m_destructor = std::move(other.m_destructor);
            other.m_handle = VK_NULL_HANDLE;
        }
        return *this;
    }

    _fox_Return_enforce bool IsValid       () const { return m_handle != VK_NULL_HANDLE; }
    _fox_Return_enforce bool IsDestructible() const { return m_destructor != nullptr;    }

private:
    T             m_handle    { VK_NULL_HANDLE };
    ResetCallback m_destructor{    nullptr     };
};

#endif //FXMEMORY_H
