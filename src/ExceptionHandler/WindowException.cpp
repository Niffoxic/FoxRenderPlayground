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

int WindowException::GetErrorCode() const
{
    return m_dwLastError;
}

std::string WindowException::GetAddOnErrorMessage() const
{
    static thread_local std::string s_errorMessage;

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
