//
// Created by niffo on 7/17/2025.
//

#ifndef WINDOWEXCEPTION_H
#define WINDOWEXCEPTION_H

#include "IException.h"
#include "Common/DefineWindows.h"

class WindowException final: public IException
{
public:
    WindowException(
        _In_z_ const char* file,
        _In_   int line,
        _In_z_ const char* func);

    WindowException(
        _In_z_ const char* file,
        _In_   int line,
        _In_z_ const char* func,
        _In_   DWORD exceptionCode);

    ~WindowException() override = default;

    _fox_Return_safe static FString GetExceptionMessageFromCode(DWORD exceptionCode);
    _fox_Return_safe static FString GetErrorMessageFromCode(DWORD exceptionCode);

protected:
    _fox_Return_safe int         GetErrorCode        () const override;
    _fox_Return_safe FString GetAddOnErrorMessage() const _Success_(return.length() > 0) override;

private:
    mutable DWORD m_dwLastError;
};

#define THROW_WINDOW_EXCEPTION()            throw WindowException(__FILE__, __LINE__, __FUNCTION__);
#define THROW_WINDOW_EXCEPTION_SYSTEM(code) throw WindowException(__FILE__, __LINE__, __FUNCTION__, code);

#endif //WINDOWEXCEPTION_H
