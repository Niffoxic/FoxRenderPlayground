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
        _In_z_ const char* file,
        _In_   int line,
        _In_z_ const char* func);

    IException(
        _In_z_ const char* file,
        _In_   int line,
        _In_z_ const char* func,
        _In_z_ const char* message);

    ~IException() override = default;

    FOX_CHECK_RETURN const char* what() const noexcept override _Success_(return != nullptr);

    void SaveCrashLog(_In_ const std::string& savePath) const;

    FOX_CHECK_RETURN const  std::string& GetFilePath     () const { return m_szErrorFileName;     }
    FOX_CHECK_RETURN int    GetErrorLine                 () const { return m_nLine;               }
    FOX_CHECK_RETURN const  std::string& GetErrorFunction() const { return m_szErrorFunctionName; }

protected:
    const   std::string& GetErrorLog         () const;
    virtual int          GetErrorCode        () const { return 0;    }
    virtual std::string  GetAddOnErrorMessage() const { return ""; }   // Add Message to the end iff its very specific

protected:
    std::string         m_szErrorFileName;
    std::string         m_szErrorMessageName; // Optional
    std::string         m_szErrorFunctionName;
    int                 m_nLine;
    mutable std::string m_szWhatBuffer;
};

#define THROW_EXCEPTION()        throw IException(__FILE__, __LINE__, __FUNCTION__);
#define THROW_EXCEPTION_MSG(msg) throw IException(__FILE__, __LINE__, __FUNCTION__, msg);
#define THROW_EXCEPTION_FMT(fmtstr, ...) \
throw IException(__FILE__, __LINE__, __FUNCTION__, std::format(fmtstr, __VA_ARGS__).c_str())


#endif //IEXCEPTION_H
