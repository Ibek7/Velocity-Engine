#ifndef JJM_LOGGER_H
#define JJM_LOGGER_H

#include <chrono>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace JJM {
namespace Debug {

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

// Log categories for filtering
enum class LogCategory {
    General,
    Graphics,
    Audio,
    Physics,
    Input,
    Network,
    AI,
    ECS,
    UI,
    Scripting,
    Performance,
    Memory,
    FileIO,
    Custom
};

// Log entry structure
struct LogEntry {
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::chrono::system_clock::time_point timestamp;
    std::thread::id threadId;
};

// Log output sink interface
class LogSink {
   public:
    virtual ~LogSink() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

// Console output sink with ANSI color support
class ConsoleSink : public LogSink {
   public:
    void write(const LogEntry& entry) override;
    void flush() override;
    void setColorEnabled(bool enabled) { useColors = enabled; }

    // ANSI color codes for better readability
    static constexpr const char* COLOR_RESET = "\033[0m";
    static constexpr const char* COLOR_TRACE = "\033[37m";       // White
    static constexpr const char* COLOR_DEBUG = "\033[36m";       // Cyan
    static constexpr const char* COLOR_INFO = "\033[32m";        // Green
    static constexpr const char* COLOR_WARNING = "\033[33m";     // Yellow
    static constexpr const char* COLOR_ERROR = "\033[31m";       // Red
    static constexpr const char* COLOR_CRITICAL = "\033[1;31m";  // Bold Red
    static constexpr const char* COLOR_TIME = "\033[90m";        // Dark Gray
    static constexpr const char* COLOR_CATEGORY = "\033[35m";    // Magenta

   private:
    bool useColors = true;
    const char* getLevelColor(LogLevel level) const;
};

// File output sink
class FileSink : public LogSink {
   public:
    FileSink(const std::string& filename);
    ~FileSink();
    void write(const LogEntry& entry) override;
    void flush() override;
    void setMaxFileSize(size_t bytes) { maxFileSize = bytes; }
    void setMaxBackups(int count) { maxBackups = count; }

   private:
    void rotateLogFile();
    std::ofstream file;
    std::string filename;
    size_t currentSize = 0;
    size_t maxFileSize = 10 * 1024 * 1024;  // 10MB default
    int maxBackups = 5;
};

// Callback sink for custom handling
class CallbackSink : public LogSink {
   public:
    using Callback = std::function<void(const LogEntry&)>;
    CallbackSink(Callback cb) : callback(cb) {}
    void write(const LogEntry& entry) override {
        if (callback) callback(entry);
    }
    void flush() override {}

   private:
    Callback callback;
};

class Logger {
   public:
    static Logger& getInstance();

    // Basic logging
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, LogCategory category, const std::string& message);
    void logWithSource(LogLevel level, LogCategory category, const std::string& message,
                       const char* file, int line, const char* function);

    // Convenience methods
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);

    // Category-specific logging
    void logGraphics(LogLevel level, const std::string& message);
    void logAudio(LogLevel level, const std::string& message);
    void logPhysics(LogLevel level, const std::string& message);
    void logNetwork(LogLevel level, const std::string& message);
    void logAI(LogLevel level, const std::string& message);
    void logPerformance(LogLevel level, const std::string& message);

    // Configuration
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    void setConsoleOutput(bool enabled);

    // Category filtering
    void setCategoryEnabled(LogCategory category, bool enabled);
    bool isCategoryEnabled(LogCategory category) const;
    void enableAllCategories();
    void disableAllCategories();

    // Sink management
    void addSink(std::unique_ptr<LogSink> sink);
    void clearSinks();

    // Log history
    void setHistorySize(size_t maxEntries);
    const std::vector<LogEntry>& getHistory() const { return history; }
    void clearHistory();
    std::vector<LogEntry> getHistoryByLevel(LogLevel level) const;
    std::vector<LogEntry> getHistoryByCategory(LogCategory category) const;

    // Formatted logging
    template <typename... Args>
    void logf(LogLevel level, const char* format, Args... args);

    void flush();

    // Utility
    static std::string levelToString(LogLevel level);
    static std::string categoryToString(LogCategory category);

   private:
    Logger();
    ~Logger();

    void writeLog(const LogEntry& entry);
    std::string formatEntry(const LogEntry& entry) const;
    std::string getCurrentTimestamp() const;

    LogLevel minLevel;
    std::ofstream logFile;
    bool consoleOutput;
    std::mutex mutex;

    // Category filtering
    std::unordered_map<LogCategory, bool> categoryEnabled;

    // Sinks
    std::vector<std::unique_ptr<LogSink>> sinks;

    // History buffer
    std::vector<LogEntry> history;
    size_t maxHistorySize = 1000;
};

// Formatted logging implementation
template <typename... Args>
void Logger::logf(LogLevel level, const char* format, Args... args) {
    char buffer[4096];
    snprintf(buffer, sizeof(buffer), format, args...);
    log(level, std::string(buffer));
}

// Enhanced macros with source location
#define LOG_TRACE(msg)                                                                     \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Trace,           \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_DEBUG(msg)                                                                     \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Debug,           \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_INFO(msg)                                                                      \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Info,            \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_WARNING(msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Warning,         \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_ERROR(msg)                                                                     \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Error,           \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_CRITICAL(msg)                                                                  \
    JJM::Debug::Logger::getInstance().logWithSource(JJM::Debug::LogLevel::Critical,        \
                                                    JJM::Debug::LogCategory::General, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)

// Category-specific macros
#define LOG_GRAPHICS(level, msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::Graphics, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_AUDIO(level, msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::Audio, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_PHYSICS(level, msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::Physics, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_NETWORK(level, msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::Network, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_AI(level, msg)                                                                   \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::AI, msg, \
                                                    __FILE__, __LINE__, __FUNCTION__)
#define LOG_PERF(level, msg)                                                                     \
    JJM::Debug::Logger::getInstance().logWithSource(level, JJM::Debug::LogCategory::Performance, \
                                                    msg, __FILE__, __LINE__, __FUNCTION__)

}  // namespace Debug
}  // namespace JJM

#endif
