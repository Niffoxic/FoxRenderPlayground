//
// Created by niffo on 7/16/2025.
//

#include "Logger.h"

#include <sstream>
#include <iomanip>

std::unique_ptr<Logger> Logger::m_pInstance = nullptr;

void Logger::Initialize(const LOGGER_INIT_DESC &desc)
{
    if (!m_pInstance)
    {
        m_pInstance = std::unique_ptr<Logger>(new Logger(desc));
    }
}

void Logger::Terminate()
{
    m_pInstance.reset();
}

Logger& Logger::Get()
{
    if (!m_pInstance)
        throw std::runtime_error("Logger is not initialized. Call Logger::Initialize() first.");
    return *m_pInstance;
}

Logger::Logger(const LOGGER_INIT_DESC& desc)
    : m_bTerminalEnabled(desc.EnableTerminal)
{
    if (desc.EnableTerminal)
    {
        EnableTerminal();
    }

    // Create log directory if it doesn't exist
    FileSystem::CreateDirectories(desc.FolderPath);

    const std::string logFilePath = std::format("{}\\{}_{}.log",
                                          desc.FolderPath,
                                          desc.FilePrefix,
                                          GetTimestamp());

    // Open custom FileSystem log file for writing
    m_logFile.OpenForWrite(logFilePath);
}

Logger::~Logger()
{
    m_logFile.Close();
}

void Logger::Log(const LogLevel level, const std::string &msg)
{
    std::scoped_lock lock(m_mutex);

    std::string levelPrefix;
    switch (level)
    {
    case LogLevel::Info:    levelPrefix = "[INFO]    "; break;
    case LogLevel::Warning: levelPrefix = "[WARNING] "; break;
    case LogLevel::Error:   levelPrefix = "[ERROR]   "; break;
    case LogLevel::Success: levelPrefix = "[SUCCESS] "; break;
    case LogLevel::Fail:    levelPrefix = "[FAIL]    "; break;
    case LogLevel::Print:   levelPrefix = "          "; break;
    }

    // Tree-style prefix (for regular lines inside current scope)
    const std::string treePrefix = BuildPrefix(/*isNodeLine=*/false);

    // If you still want legacy tab support for places where you used LOG_ADD_TAB/REMOVE_TAB,
    // you can append tabs AFTER the tree prefix (or remove m_nTabs usage entirely).
    std::string tabPrefix(m_nTabs, '\t');

    const std::string fullMsg = std::format("{}{}{}{}\n", treePrefix, tabPrefix, levelPrefix, msg);

    // Print to terminal
    if (m_bTerminalEnabled && m_hConsole)
    {
        SetConsoleColor(level);
        DWORD written;
        WriteConsoleA(m_hConsole, fullMsg.c_str(), static_cast<DWORD>(fullMsg.length()), &written, nullptr);
        SetConsoleTextAttribute(m_hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }

    // Write to log file
    if (m_logFile.IsOpen())
    {
        m_logFile.WritePlainText(fullMsg);
    }
}

std::string Logger::GetTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
    localtime_s(&localTm, &timeT);

    std::ostringstream oss;
    oss << std::put_time(&localTm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void Logger::SetConsoleColor(const LogLevel level) const
{
    WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

    switch (level)
    {
    case LogLevel::Info:
        color = FOREGROUND_GREEN | FOREGROUND_BLUE; // Cyan
        break;
    case LogLevel::Warning:
        color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
        break;
    case LogLevel::Error:
        color = FOREGROUND_RED | FOREGROUND_INTENSITY; // Bright Red
        break;
    case LogLevel::Success:
        color = FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Bright Green
        break;
    case LogLevel::Fail:
        color = FOREGROUND_RED | FOREGROUND_BLUE; // Magenta-ish
        break;
    case LogLevel::Print:
        color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // Default
        break;
    }

    SetConsoleTextAttribute(m_hConsole, color);
}

void Logger::EnableTerminal()
{
    if (!AllocConsole()) return;

    FILE* dummy;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$",  "r", stdin);

    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    m_bTerminalEnabled = true;
    SetConsoleOutputCP(CP_UTF8);
}


void Logger::BeginScopeImpl(const std::string& name, bool hasNextSibling)
{
    const std::string nodePrefix = BuildPrefix(true);
    Log(LogLevel::Print, nodePrefix + name);

    // Descend one level; remember fox whether THIS level still has siblings
    m_scopeHasNext.push_back(hasNextSibling);
}

void Logger::EndScopeImpl()
{
    if (!m_scopeHasNext.empty())
        m_scopeHasNext.pop_back();
}

std::string Logger::BuildPrefix(const bool isNodeLine) const
{
    // Style chars
    const bool unicode = (m_indentStyle == IndentStyle::Unicode);
    const char* V  = unicode ? "│   " : "|   ";
    const char* T  = unicode ? "├── " : "|-- ";
    const char* L  = unicode ? "└── " : "`-- ";
    const char* SP = "    ";

    std::string pre;

    // Draw vertical guides for ancestor levels
    if (!m_scopeHasNext.empty())
    {
        for (size_t i = 0; i + 1 < m_scopeHasNext.size(); ++i)
            pre += (m_scopeHasNext[i] ? V : SP);

        // For the current level:
        if (isNodeLine) {
            // node header uses ├── or └──
            pre += (m_scopeHasNext.back() ? T : L);
        } else {
            // regular log line inside scope: keep vertical if ancestors have next
            pre += (m_scopeHasNext.back() ? V : SP);
        }
    }

    return pre;
}
