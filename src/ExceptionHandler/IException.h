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
    IException(const char* file, int line, const char* func, const char* message="No Details");
    ~IException() override = default;

    virtual const char* what() const noexcept override;
    void SaveCrashLog(const std::string& savePath) const;

    const std::string& GetFilePath() const { return m_szErrorFileName; }
    int GetErrorLine() const { return m_nLine; }
    const std::string& GetErrorFunction() const { return m_szErrorFunctionName; }
    const std::string& GetErrorMessage() const { return m_szErrorMessage; }

protected:
    const std::string& GetErrorLog() const;

    virtual const std::string& GetAddOnErrorMessage() const { return ""; }   // Add Message to the end iff its very specific

protected:
    std::string m_szErrorFileName;
    int m_nLine;
    std::string m_szErrorFunctionName;
    std::string m_szErrorMessage;
    mutable std::string m_szWhatBuffer;
};

#define THROW_EXCEPTION(msg) throw IException(__FILE__, __LINE__, __FUNCTION__, msg);
#define THROW() throw IException(__FILE__, __LINE__, __FUNCTION__);

#endif //IEXCEPTION_H
