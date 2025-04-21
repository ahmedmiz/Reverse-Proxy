#include "Logger.h"
#include <stdexcept>
#include <sys/stat.h>
#include <filesystem>
namespace fs = std::filesystem;

Logger::~Logger()
{

    for (auto &[level, file] : logFiles_)
    {
        if (file.is_open())
            file.close();
    }
}

Logger &Logger::getInstance()
{
    static Logger logger;
    return logger;
}

void Logger::init(const std::string &logDir, Level minLevel)
{
    std::cout << "Intializing the logger" << std::endl;
    std::lock_guard<std::mutex> lock(logMutex_);
    minLevel_ = minLevel;
    logDir_ = logDir;
    ensureLogDirectoryExists();
    for (int i = 0; i <= static_cast<int>(Level::CRITICAL); ++i)
    {
        Level level = static_cast<Level>(i);
        std::string filePath = logDir_ + "/" + getLogFileName(level);
        logFiles_[level].open(filePath, std::ios::out | std::ios::app);
        if (!logFiles_[level].is_open())
        {
            throw std::runtime_error("Failed to open log file: " + filePath);
        }
        logFiles_[level] << "Log file opened at " << getCurrentTime() << std::endl;
    }
}
void Logger::log(Level level, const std::string &message, const std::string &source)
{
    if (level < minLevel_)
        return;
    std::ostringstream oss;
    oss << "[" << getCurrentTime() << "] " << levelToString(level) << " ";
    if (!source.empty())
        oss << source << ": ";
    oss << message << std::endl;
    writeToLog(level, oss.str());

    if (level >= Level::ERROR)
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        std::cerr << oss.str() << std::endl;
    }
}
void Logger::debug(const std::string &message, const std::string &source)
{
    log(Level::DEBUG, message, source);
}

void Logger::info(const std::string &message, const std::string &source)
{
    log(Level::INFO, message, source);
}

void Logger::warning(const std::string &message, const std::string &source)
{
    log(Level::WARNING, message, source);
}

void Logger::error(const std::string &message, const std::string &source)
{
    log(Level::ERROR, message, source);
}

void Logger::critical(const std::string &message, const std::string &source)
{
    log(Level::CRITICAL, message, source);
}
void Logger::flush()
{
    std::lock_guard<std::mutex> lock(logMutex_);
    for (auto &[level, file] : logFiles_)
    {
        if (file.is_open())
        {
            file.flush();
        }
    }
}
std::string Logger::levelToString(Level level)
{
    switch (level)
    {
    case Level::DEBUG:
        return "DEBUG";
    case Level::INFO:
        return "INFO";
    case Level::WARNING:
        return "WARNING";
    case Level::ERROR:
        return "ERROR";
    case Level::CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}
std::string Logger::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&in_time_t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}
std::string Logger::getLogFileName(Level level)
{
    switch (level)
    {
    case Level::DEBUG:
        return "debug.log";
    case Level::INFO:
        return "info.log";
    case Level::WARNING:
        return "warning.log";
    case Level::ERROR:
        return "error.log";
    case Level::CRITICAL:
        return "critical.log";
    default:
        return "unknown.log";
    }
}
void Logger::ensureLogDirectoryExists()
{
    std::cout << "making the LogDir_" << std::endl;
    if (!fs::exists(logDir_))
    {
        std::cout << "LogDir_ does not exist" << std::endl;
        if (!fs::create_directories(logDir_))
        {
            throw std::runtime_error("Failed to create log directory: " + logDir_);
        }
    }
}
void Logger::writeToLog(Level level, const std::string &formattedMessage)
{
    std::lock_guard<std::mutex> lock(logMutex_);

    if (logFiles_[level].is_open())
    {
        logFiles_[level] << formattedMessage << std::endl;
    }
}
void Logger::setMinLevel(Level level)
{
    std::lock_guard<std::mutex> lock(logMutex_);
    minLevel_ = level;
}