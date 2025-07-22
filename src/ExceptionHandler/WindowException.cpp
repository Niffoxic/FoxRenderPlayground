//
// Created by niffo on 7/17/2025.
//

#include "WindowException.h"

#include "Common/DefineWindows.h"

WindowException::WindowException(const char *file, int line, const char *func)
: IException(file, line, func)
{
    m_dwLastError = GetLastError();
}

WindowException::WindowException(const char *file, int line, const char *func, DWORD exceptionCode)
: IException(file, line, func)
{
    m_dwLastError = exceptionCode;
}

FString WindowException::GetExceptionMessageFromCode(DWORD exceptionCode)
{
    LPVOID lpMsgBuf = nullptr;

    constexpr DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS;

    constexpr DWORD langId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);

    FString result;

#if defined(FOX_STRING_IS_ANSI)
    const DWORD size = FormatMessageA(
        flags,
        nullptr,
        exceptionCode,
        langId,
        reinterpret_cast<LPSTR>(&lpMsgBuf),
        0,
        nullptr
    );

    if (size && lpMsgBuf)
    {
        result = std::string(static_cast<LPSTR>(lpMsgBuf), size);
        std::erase(result, '\n');
        std::erase(result, '\r');

        // Optional trim for trailing whitespace
        result.erase(
            std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            result.end()
        );
    }
    else
    {
        result = "Unknown SEH Exception Code: " + std::to_string(exceptionCode);
    }
#else
    const DWORD size = FormatMessageW(
        flags,
        nullptr,
        exceptionCode,
        langId,
        reinterpret_cast<LPWSTR>(&lpMsgBuf),
        0,
        nullptr
    );

    if (size && lpMsgBuf)
    {
        result = std::wstring(static_cast<LPWSTR>(lpMsgBuf), size);
        std::erase(result, L'\n');
        std::erase(result, L'\r');

        // Optional trim for trailing whitespace
        result.erase(
            std::find_if(result.rbegin(), result.rend(), [](wchar_t ch) {
                return !std::iswspace(ch);
            }).base(),
            result.end()
        );
    }
    else
    {
        result = L"Unknown SEH Exception Code: " + std::to_wstring(exceptionCode);
    }
#endif

    LocalFree(lpMsgBuf);
    return result;
}

FString WindowException::GetErrorMessageFromCode(DWORD exceptionCode)
{
    thread_local FString s_errorMessage;

    if (exceptionCode == 0)
    {
        s_errorMessage = F_TEXT("No additional Windows error information.");
        return s_errorMessage;
    }

#if defined(FOX_STRING_IS_ANSI)
    LPSTR messageBuffer = nullptr;
    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        exceptionCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr
    );

    if (size && messageBuffer)
    {
        s_errorMessage = FString(messageBuffer, messageBuffer + size);
        std::erase(s_errorMessage, '\n');
        std::erase(s_errorMessage, '\r');
    }
    else
    {
        s_errorMessage = F_TEXT("Unknown error code: ") + ToFString(exceptionCode);
    }

    LocalFree(messageBuffer);

#else
    LPWSTR messageBuffer = nullptr;
    const DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        exceptionCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&messageBuffer),
        0,
        nullptr
    );

    if (size && messageBuffer)
    {
        s_errorMessage = FString(messageBuffer, messageBuffer + size);
        std::erase(s_errorMessage, L'\n');
        std::erase(s_errorMessage, L'\r');
    }
    else
    {
        s_errorMessage = F_TEXT("Unknown error code: ") + TO_FSTRING(exceptionCode);
    }

    LocalFree(messageBuffer);
#endif

    return s_errorMessage;
}

int WindowException::GetErrorCode() const
{
    return static_cast<int>(m_dwLastError);
}

std::string WindowException::GetAddOnErrorMessage() const
{
    thread_local std::string s_errorMessage;

    if (m_dwLastError == 0)
    {
        s_errorMessage = "No additional Windows error information.";
        return s_errorMessage;
    }

    LPSTR messageBuffer = nullptr;

    const DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        m_dwLastError,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&messageBuffer),
        0,
        nullptr
    );

    if (size && messageBuffer)
    {
        s_errorMessage = std::string(messageBuffer, size);
        std::erase(s_errorMessage, '\n');
        std::erase(s_errorMessage, '\r');
    }
    else
    {
        s_errorMessage = "Unknown error code: " + std::to_string(m_dwLastError);
    }

    LocalFree(messageBuffer);
    return s_errorMessage;
}
