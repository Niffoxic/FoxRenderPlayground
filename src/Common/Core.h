//
// Created by niffo on 7/20/2025.
//

#ifndef CORE_H
#define CORE_H

#pragma region ANNOTATIONS
#ifdef _MSC_VER
    #include <sal.h>
    #include <vcruntime.h>

    #define FOX_CHECK_RETURN     _Check_return_ _NODISCARD
    #define FOX_IN               _In_
    #define FOX_OUT              _Out_
    #define FOX_INOUT            _Inout_
    #define FOX_SUCCESS(x)       _Success_(x)

#elif defined(__clang__) || defined(__GNUC__)
    #define FOX_CHECK_RETURN     [[nodiscard]]
    #define FOX_IN
    #define FOX_OUT
    #define FOX_INOUT
    #define FOX_SUCCESS(x)
#else
    #warning "Unknown compiler; FOX_* annotations will be empty."
    #define FOX_CHECK_RETURN
    #define FOX_IN
    #define FOX_OUT
    #define FOX_INOUT
    #define FOX_SUCCESS(x)
#endif

#pragma endregion ANNOTATIONS

#pragma region CPP_SPECIFICS

/**
 *      STRING SPECIFICS
 */

#if defined(FOX_STRING_IS_ANSI)
    #include <string>
    using FString = std::string;
    #define F_TEXT(str) str
#else
    #include <string>
    #include <cstring>
    using FString = std::wstring;
    #define F_TEXT(str) L##str
#endif

inline FString ToFString(const char* str)
{
#if defined(FOX_STRING_IS_ANSI)
    return std::string(str);
#else
    size_t len = std::strlen(str);
    return std::wstring(str, str + len);
#endif
}

template<typename T>
FString ToFString(T value)
{
#if defined(FOX_STRING_IS_ANSI)
    return std::to_string(value);
#else
    return std::to_wstring(value);
#endif
}

#pragma endregion CPP_SPECIFICS

#endif //CORE_H
