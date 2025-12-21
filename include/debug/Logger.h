#ifndef JJM_LOGGER_H
#define JJM_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <vector>

namespace JJM {
namespace Debug {

enum class LogLevel { Trace, Debug, Info, Warning, Error, Critical };

class Logger {
public:
    static Logger& getInstance();
    
    void log(LogLevel level, const std::string& message);
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    void setLogLevel(LogLevel level);
    void setLogFile(const std::string& filename);
    void setConsoleOutput(bool enabled);
    
    void flush();

private:
    Logger();
    ~Logger();
    
    void writeLog(LogLevel level, const std::string& message);
    std::string levelToString(LogLevel level) const;
    std::string getCurrentTimestamp() const;
    
    LogLevel minLevel;
    std::ofstream logFile;
    bool consoleOutput;
    std::mutex mutex;
};

#define LOG_TRACE(msg) JJM::Debug::Logger::getInstance().trace(msg)
#define LOG_DEBUG(msg) JJM::Debug::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) JJM::Debug::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) JJM::Debug::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) JJM::Debug::Logger::getInstance().error(msg)
#define LOG_CRITICAL(msg) JJM::Debug::Logger::getInstance().critical(msg)

} // namespace Debug
} // namespace JJM

#endif
