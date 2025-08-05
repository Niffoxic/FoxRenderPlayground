//
// Created by niffo on 7/23/2025.
//

#ifndef ISINGLETON_H
#define ISINGLETON_H

#include "Common/Core.h"
#include "Logger/Logger.h"
#include "ExceptionHandler/IException.h"

#include <mutex>


template<typename T, typename... Args>
class ISingleton
{
public:
    ISingleton(ISingleton&&)      = delete;
    ISingleton(const ISingleton&) = delete;

    ISingleton& operator=(const ISingleton&) = delete;
    ISingleton& operator=(ISingleton&&)      = delete;

    template<typename... TArgs>
    _fox_Return_enforce static T& Get(_fox_In_ TArgs&&...args)
    _fox_Success_(s_pInstance != nullptr);

    static void Destroy()
    {
        s_pInstance.reset();
    }

protected:
    explicit ISingleton(Args&... args) = default;
    virtual ~ISingleton()              = default;

private:
    inline static std::unique_ptr<T> s_pInstance{ nullptr };
    inline static std::once_flag s_onceFlag;
};

template<typename T, typename ... Args>
template<typename ... TArgs>
T & ISingleton<T, Args...>::Get(TArgs &&...args)
{
    std::call_once(s_onceFlag, [&]()
{
    s_pInstance.reset(new T(std::forward<TArgs>(args)...));
});

    if (s_pInstance == nullptr)
    {
#if defined(_DEBUG) || defined(DEBUG)
        __debugbreak();
#else
        FString msg = F_TEXT("Failed t0 get installer of ") + F_TEXT(#T)
        THROW_EXCEPTION_MSG(msg.c_str());
#endif
    }
    return *s_pInstance;
}

#endif //ISINGLETON_H
