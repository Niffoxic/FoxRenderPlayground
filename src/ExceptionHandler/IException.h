//
// Created by niffo on 7/17/2025.
//

#ifndef IEXCEPTION_H
#define IEXCEPTION_H

#include "Common/Core.h"
#include <stdexcept>
#include <format>
#include <string>

class IException: public std::exception
{
public:
    IException(
        _fox_In_z_ const char* file,
        _fox_In_   int line,
        _fox_In_z_ const char* func);

    IException(
        _fox_In_z_ const char* file,
        _fox_In_   int line,
        _fox_In_z_ const char* func,
        _fox_In_z_ const char* message);

    ~IException() override = default;

    _fox_Return_enforce const char* what() const noexcept override _fox_Success_(return != nullptr);

    void SaveCrashLog(_In_ const FString& savePath) const;

    _fox_Return_enforce const  FString& GetFilePath     () const { return m_szErrorFileName;     }
    _fox_Return_enforce int    GetErrorLine             () const { return m_nLine;               }
    _fox_Return_enforce const  FString& GetErrorFunction() const { return m_szErrorFunctionName; }

protected:
    const   FString& GetErrorLog         () const;
    virtual int          GetErrorCode    () const { return 0;    }
    virtual FString  GetAddOnErrorMessage() const { return ""; }   // Add Message to the end iff its very specific

protected:
    FString         m_szErrorFileName;
    FString         m_szErrorMessageName;
    FString         m_szErrorFunctionName;
    mutable FString m_szWhatBuffer;
    int             m_nLine;
};

#define THROW_EXCEPTION()        throw IException(__FILE__, __LINE__, __FUNCTION__);
#define THROW_EXCEPTION_MSG(msg) throw IException(__FILE__, __LINE__, __FUNCTION__, msg);
#define THROW_EXCEPTION_FMT(fmtstr, ...) \
throw IException(__FILE__, __LINE__, __FUNCTION__, std::format(fmtstr, __VA_ARGS__).c_str())


#endif //IEXCEPTION_H
