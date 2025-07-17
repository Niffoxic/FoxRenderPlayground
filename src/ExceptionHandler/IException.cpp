//
// Created by niffo on 7/17/2025.
//

#include "IException.h"
#include "FileSystem/FileSystem.h"
#include "Logger/Logger.h"

#include <sstream>

IException::IException(const char *file, int line, const char *func)
:   m_szErrorFileName(file),
    m_nLine(line),
    m_szErrorFunctionName(func)
{}

IException::IException(const char *file, int line, const char *func, const char *message)
:   m_szErrorFileName(file),
    m_nLine(line),
    m_szErrorFunctionName(func),
    m_szErrorMessageName(message)
{}

const char * IException::what() const noexcept
{
    return GetErrorLog().c_str();
}

const std::string& IException::GetErrorLog() const
{
    if (m_szWhatBuffer.empty())
    {
        std::ostringstream os;
        os
        << "[Exception]\nAt Function: " << m_szErrorFunctionName
        << "\nFile: " << m_szErrorFileName << "\nLine: " << m_nLine;

        if (const std::string addedMessage = GetAddOnErrorMessage(); not addedMessage.empty())
        {
            os << "\n[Error Code]: " << GetErrorCode();
            os << "\n[Details]: " << addedMessage;
        }

        if (not m_szErrorMessageName.empty())
        {
            os << "\n[Message]: " << m_szErrorMessageName;
        }

        m_szWhatBuffer = os.str();
    }
    return m_szWhatBuffer;
}

void IException::SaveCrashLog(const std::string& savePath) const
{
    //~ TODO: Make it dynamic later......
    std::string path = savePath;
    if (not path.ends_with("/")) path += "/";
    path += "log_" + Logger::GetTimestamp() + ".txt";
    FileSystem fs{};
    fs.OpenForWrite(path);
    fs.WritePlainText(GetErrorLog());
    fs.Close();
}
