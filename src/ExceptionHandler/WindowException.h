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
    WindowException(const char* file, int line, const char* func);
    ~WindowException() override = default;

protected:
    int GetErrorCode() const override;
    std::string GetAddOnErrorMessage() const override;

private:
    mutable DWORD m_dwLastError;
};

#define THROW_WINDOW_EXCEPTION() throw WindowException(__FILE__, __LINE__, __FUNCTION__);

#endif //WINDOWEXCEPTION_H
