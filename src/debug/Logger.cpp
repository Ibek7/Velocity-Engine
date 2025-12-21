#include "debug/Logger.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace JJM {
namespace Debug {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : minLevel(LogLevel::Trace), consoleOutput(true) {
}

Logger::~Logger() {
    flush();
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel) return;
    writeLog(level, message);
}

void Logger::trace(const std::string& message) {
    log(LogLevel::Trace, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::Debug, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::Info, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::Warning, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::Error, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::Critical, message);
}

void Logger::setLogLevel(LogLevel level) {
    minLevel = level;
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (logFile.is_open()) {
        logFile.close();
    }
    
    logFile.open(filename, std::ios::app);
}

void Logger::setConsoleOutput(bool enabled) {
    consoleOutput = enabled;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
        logFile.flush();
    }
}

void Logger::writeLog(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    std::string formatted = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    if (consoleOutput) {
        std::cout << formatted << std::endl;
    }
    
    if (logFile.is_open()) {
        logFile << formatted << std::endl;
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info: return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error: return "ERROR";
        case LogLevel::Critical: return "CRIT";
        default: return "UNKNOWN";
    }
}

std::string Logger::getCurrentTimestamp() const {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S");
    
    return oss.str();
}

} // namespace Debug
} // namespace JJM
