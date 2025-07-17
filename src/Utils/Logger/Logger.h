//
// Created by niffo on 7/16/2025.
//

#ifndef LOGGER_H
#define LOGGER_H

#include "Common/DefineWindows.h"
#include "FileSystem/FileSystem.h"

#include <string>
#include <mutex>
#include <format>

enum class LogLevel: uint8_t
{
    Info,
    Warning,
    Error,
    Success,
    Fail,
    Print
};

typedef struct LOGGER_INIT_DESC
{
    std::string FolderPath;
    std::string FilePrefix;
    bool EnableTerminal = false;
}LOGGER_INIT_DESC;

/**
 * @brief Windows-specific, thread-safe singleton logger.
 */
class Logger
{
public:
    ~Logger();

    static void Initialize(const LOGGER_INIT_DESC& desc);
    static void Terminate();
    static bool IsInitialized() { return m_pInstance != nullptr; }
    static Logger& Get();

    template<typename... Args>
        static void Info(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Info, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Warning(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Warning, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Error(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Error, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Success(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Success, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Fail(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Fail, std::format(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    static void Print(std::format_string<Args...> fmt, Args&&... args)
    {
        if (!IsInitialized()) return;
        Get().Log(LogLevel::Print, std::format(fmt, std::forward<Args>(args)...));
    }

    static std::string GetTimestamp();

private:
    explicit Logger(const LOGGER_INIT_DESC& desc);

    void Log(LogLevel level, const std::string& msg);
    void SetConsoleColor(LogLevel level) const;
    void EnableTerminal();

private:
    static std::unique_ptr<Logger> m_pInstance;
    std::mutex m_mutex;
    HANDLE m_hConsole{ nullptr };
    bool m_bTerminalEnabled{ false };
    FileSystem m_logFile{};
};

// global access macros
#define INIT_GLOBAL_LOGGER(desc)   Logger::Initialize(desc)
#define TERMINATE_GLOBAL_LOGGER()  Logger::Terminate()

#define LOG_INFO(...)    Logger::Info(__VA_ARGS__)
#define LOG_PRINT(...)   Logger::Print(__VA_ARGS__)
#define LOG_WARNING(...) Logger::Warning(__VA_ARGS__)
#define LOG_ERROR(...)   Logger::Error(__VA_ARGS__)
#define LOG_SUCCESS(...) Logger::Success(__VA_ARGS__)
#define LOG_FAIL(...)    Logger::Fail(__VA_ARGS__)

#endif //LOGGER_H
