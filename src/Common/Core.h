//
// Created by niffo on 7/20/2025.
//

#ifndef CORE_H
#define CORE_H

#pragma region ANNOTATIONS
#ifdef _MSC_VER
    #include <sal.h>
    #include <vcruntime.h>

    #define _fox_Return_safe        _Check_return_ _NODISCARD
    #define _fox_In_                _In_
    #define _fox_In_z_              _In_z_
    #define _fox_Out_               _Out_
    #define _fox_Inout_             _Inout_
    #define _fox_Success_(x)        _Success_(x)
    #define _fox_Ret_maybenull_     _Ret_maybenull_
    #define _fox_Pre_satisfies_(x)  _Pre_satisfies_(x)

#elif defined(__clang__) || defined(__GNUC__)
#warning "You building without MSCV don't blame me if dont work"

    #define _fox_Return_safe        [[nodiscard]]
    #define _fox_In_                _In_
    #define _fox_Out_               _Out_
    #define _fox_Inout_             _Inout_
    #define _fox_Success_(x)        _Success_(x)
    #define _fox_Ret_maybenull_     _Ret_maybenull_
    #define _fox_Pre_satisfies_(x)
#endif

#pragma endregion ANNOTATIONS

#pragma region CPP_SPECIFICS

#if defined(_MSC_VER)
    #define _NOVTABLE __declspec(novtable)
#else
    #define _NOVTABLE
#endif

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
