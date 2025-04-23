#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <chrono>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

class Logger {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL
    };
    // singleton instance return it by reference 
    static Logger& getInstance();
    // Prevent accidental copying of the singleton.
    Logger(const Logger&) = delete;
    // Prevent assignment of the singleton.
    Logger& operator=(const Logger&) = delete;
    

    void init(const std::string& logDir = "logs", Level minLevel = Level::INFO);
    void log(Level level, const std::string& message, const std::string& source = "");
    
    // Convenience methods
    void debug(const std::string& message, const std::string& source = "");
    void info(const std::string& message, const std::string& source = "");
    void warning(const std::string& message, const std::string& source = "");
    void error(const std::string& message, const std::string& source = "");
    void critical(const std::string& message, const std::string& source = "");

    void setMinLevel(Level level);
    void flush();

private:
    Logger() = default;
    ~Logger();

    std::mutex logMutex_;
    std::map<Level, std::ofstream> logFiles_;
    Level minLevel_ = Level::INFO;
    std::string logDir_;

    std::string levelToString(Level level);
    std::string getCurrentTime();
    std::string getLogFileName(Level level);
    void ensureLogDirectoryExists();
    void writeToLog(Level level, const std::string& formattedMessage);
};