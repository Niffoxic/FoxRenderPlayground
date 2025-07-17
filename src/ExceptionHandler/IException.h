//
// Created by niffo on 7/17/2025.
//

#ifndef IEXCEPTION_H
#define IEXCEPTION_H

#include <stdexcept>
#include <string>

class IException: public std::exception
{
public:
    IException(const char* file, int line, const char* func);
    IException(const char* file, int line, const char* func, const char* message);
    ~IException() override = default;

    const char* what() const noexcept override;
    void SaveCrashLog(const std::string& savePath) const;

    const std::string& GetFilePath() const { return m_szErrorFileName; }
    int GetErrorLine() const { return m_nLine; }
    const std::string& GetErrorFunction() const { return m_szErrorFunctionName; }

protected:
    const std::string& GetErrorLog() const;
    virtual int GetErrorCode() const { return 0; }
    virtual std::string GetAddOnErrorMessage() const { return ""; }   // Add Message to the end iff its very specific

protected:
    std::string m_szErrorFileName;
    std::string m_szErrorMessageName; // Optional
    int m_nLine;
    std::string m_szErrorFunctionName;
    mutable std::string m_szWhatBuffer;
};

#define THROW_EXCEPTION() throw IException(__FILE__, __LINE__, __FUNCTION__);
#define THROW_EXCEPTION_MSG(msg) throw IException(__FILE__, __LINE__, __FUNCTION__, msg);

#endif //IEXCEPTION_H
