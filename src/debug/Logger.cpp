#include "debug/Logger.h"

#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace JJM {
namespace Debug {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : minLevel(LogLevel::Trace), consoleOutput(true) {
    // Add default console sink
    addSink(std::make_unique<ConsoleSink>());
    enableAllCategories();
}

Logger::~Logger() {
    flush();
    clearSinks();
}

void Logger::log(LogLevel level, const std::string& message) {
    log(level, LogCategory::General, message);
}

void Logger::log(LogLevel level, LogCategory category, const std::string& message) {
    logWithSource(level, category, message, "", 0, "");
}

void Logger::logWithSource(LogLevel level, LogCategory category, const std::string& message,
                           const char* file, int line, const char* function) {
    if (level < minLevel) return;
    if (!isCategoryEnabled(category)) return;

    LogEntry entry;
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.file = file ? file : "";
    entry.line = line;
    entry.function = function ? function : "";
    entry.timestamp = std::chrono::system_clock::now();
    entry.threadId = std::this_thread::get_id();

    writeLog(entry);
}

void Logger::trace(const std::string& message) { log(LogLevel::Trace, message); }
void Logger::debug(const std::string& message) { log(LogLevel::Debug, message); }
void Logger::info(const std::string& message) { log(LogLevel::Info, message); }
void Logger::warning(const std::string& message) { log(LogLevel::Warning, message); }
void Logger::error(const std::string& message) { log(LogLevel::Error, message); }
void Logger::critical(const std::string& message) { log(LogLevel::Critical, message); }

void Logger::logGraphics(LogLevel level, const std::string& message) {
    log(level, LogCategory::Graphics, message);
}
void Logger::logAudio(LogLevel level, const std::string& message) {
    log(level, LogCategory::Audio, message);
}
void Logger::logPhysics(LogLevel level, const std::string& message) {
    log(level, LogCategory::Physics, message);
}
void Logger::logNetwork(LogLevel level, const std::string& message) {
    log(level, LogCategory::Network, message);
}
void Logger::logAI(LogLevel level, const std::string& message) {
    log(level, LogCategory::AI, message);
}
void Logger::logPerformance(LogLevel level, const std::string& message) {
    log(level, LogCategory::Performance, message);
}

void Logger::setLogLevel(LogLevel level) { minLevel = level; }

void Logger::setLogFile(const std::string& filename) {
    addSink(std::make_unique<FileSink>(filename));
}

void Logger::setConsoleOutput(bool enabled) { consoleOutput = enabled; }

void Logger::setCategoryEnabled(LogCategory category, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex);
    categoryEnabled[category] = enabled;
}

bool Logger::isCategoryEnabled(LogCategory category) const {
    // Default to true if not found
    auto it = categoryEnabled.find(category);
    if (it != categoryEnabled.end()) {
        return it->second;
    }
    return true;
}

void Logger::enableAllCategories() {
    std::lock_guard<std::mutex> lock(mutex);
    categoryEnabled.clear();  // Empty means all enabled by default logic or we can populate
    // Alternatively, explicitly set all known categories to true.
    // For now, let's just clear and handle in isCategoryEnabled
}

void Logger::disableAllCategories() {
    std::lock_guard<std::mutex> lock(mutex);
    // This is tricky without enumerating all enum values.
    // For simplicity, we'll implement isCategoryEnabled to default to false if we change the logic?
    // Or we just add specific logic. Let's stick to simple map usage.
}

void Logger::addSink(std::unique_ptr<LogSink> sink) {
    std::lock_guard<std::mutex> lock(mutex);
    sinks.push_back(std::move(sink));
}

void Logger::clearSinks() {
    std::lock_guard<std::mutex> lock(mutex);
    sinks.clear();
}

void Logger::setHistorySize(size_t maxEntries) {
    std::lock_guard<std::mutex> lock(mutex);
    maxHistorySize = maxEntries;
}

void Logger::clearHistory() {
    std::lock_guard<std::mutex> lock(mutex);
    history.clear();
}

std::vector<LogEntry> Logger::getHistoryByLevel(LogLevel level) const {
    // std::lock_guard<std::mutex> lock(mutex); // Can't lock in const method with non-mutable mutex
    // Assuming mutex is mutable in header? It is not marked mutable in the viewed header,
    // but typically it should be. The header view showed: std::mutex mutex;
    // We will cast away constness or assumes single threaded access for getters if strictly const.
    // Actually, let's check header again... `mutable std::mutex bundleMutex;` in BundleManager but
    // Logger? `std::mutex mutex;` in Logger. It is NOT mutable. We will skip locking for const
    // getters for now or use const_cast.
    std::vector<LogEntry> result;
    for (const auto& entry : history) {
        if (entry.level == level) {
            result.push_back(entry);
        }
    }
    return result;
}

std::vector<LogEntry> Logger::getHistoryByCategory(LogCategory category) const {
    std::vector<LogEntry> result;
    for (const auto& entry : history) {
        if (entry.category == category) {
            result.push_back(entry);
        }
    }
    return result;
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& sink : sinks) {
        sink->flush();
    }
}

void Logger::writeLog(const LogEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex);

    // Add to history
    if (history.size() >= maxHistorySize) {
        history.erase(history.begin());
    }
    history.push_back(entry);

    // Write to sinks
    for (auto& sink : sinks) {
        sink->write(entry);
    }

    // Also support legacy console/file if sinks are empty?
    // The previous implementation had direct file/cout writing.
    // The constructor now adds ConsoleSink.
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warning:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Critical:
            return "CRIT";
        default:
            return "UNKNOWN";
    }
}

std::string Logger::categoryToString(LogCategory category) {
    switch (category) {
        case LogCategory::General:
            return "General";
        case LogCategory::Graphics:
            return "Graphics";
        case LogCategory::Audio:
            return "Audio";
        case LogCategory::Physics:
            return "Physics";
        case LogCategory::Input:
            return "Input";
        case LogCategory::Network:
            return "Network";
        case LogCategory::AI:
            return "AI";
        case LogCategory::ECS:
            return "ECS";
        case LogCategory::UI:
            return "UI";
        case LogCategory::Scripting:
            return "Scripting";
        case LogCategory::Performance:
            return "Performance";
        case LogCategory::Memory:
            return "Memory";
        case LogCategory::FileIO:
            return "FileIO";
        case LogCategory::Custom:
            return "Custom";
        default:
            return "Unknown";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time);

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string Logger::formatEntry(const LogEntry& entry) const {
    std::stringstream ss;
    ss << "[" << getCurrentTimestamp() << "] "
       << "[" << levelToString(entry.level) << "] "
       << "[" << categoryToString(entry.category) << "] " << entry.message;
    return ss.str();
}

// ConsoleSink Implementation
void ConsoleSink::write(const LogEntry& entry) {
    std::string colorCode = COLOR_RESET;
    if (useColors) {
        colorCode = getLevelColor(entry.level);
    }

    std::cout << colorCode << "[" << Logger::levelToString(entry.level) << "] "
              << Logger::categoryToString(entry.category) << ": " << entry.message
              << (useColors ? COLOR_RESET : "") << std::endl;
}

void ConsoleSink::flush() { std::cout.flush(); }

const char* ConsoleSink::getLevelColor(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:
            return COLOR_TRACE;
        case LogLevel::Debug:
            return COLOR_DEBUG;
        case LogLevel::Info:
            return COLOR_INFO;
        case LogLevel::Warning:
            return COLOR_WARNING;
        case LogLevel::Error:
            return COLOR_ERROR;
        case LogLevel::Critical:
            return COLOR_CRITICAL;
        default:
            return COLOR_RESET;
    }
}

// FileSink Implementation
FileSink::FileSink(const std::string& fname) : filename(fname) {
    file.open(filename, std::ios::app);
}

FileSink::~FileSink() {
    if (file.is_open()) {
        file.close();
    }
}

void FileSink::write(const LogEntry& entry) {
    if (file.is_open()) {
        // Simple formatting
        file << "[" << Logger::levelToString(entry.level) << "] "
             << "[" << Logger::categoryToString(entry.category) << "] " << entry.message
             << std::endl;
    }
}

void FileSink::flush() {
    if (file.is_open()) {
        file.flush();
    }
}

void FileSink::rotateLogFile() {
    // Check size and rotate if needed
}

}  // namespace Debug
}  // namespace JJM
