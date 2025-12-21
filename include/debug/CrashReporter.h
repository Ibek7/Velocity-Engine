#ifndef JJM_CRASH_REPORTER_H
#define JJM_CRASH_REPORTER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace JJM {
namespace Debug {

struct CrashInfo {
    std::string message;
    std::string stackTrace;
    std::string timestamp;
    std::string version;
    std::map<std::string, std::string> metadata;
};

class CrashReporter {
public:
    static CrashReporter& getInstance();
    
    void initialize();
    void shutdown();
    
    void reportCrash(const std::string& message);
    void reportCrash(const std::string& message, const std::string& stackTrace);
    
    void addMetadata(const std::string& key, const std::string& value);
    void setVersion(const std::string& version);
    void setCrashDirectory(const std::string& dir);
    
    using CrashCallback = std::function<void(const CrashInfo&)>;
    void setCrashCallback(CrashCallback callback);
    
    std::vector<CrashInfo> getRecentCrashes() const;

private:
    CrashReporter();
    ~CrashReporter();
    
    void writeCrashReport(const CrashInfo& info);
    std::string generateStackTrace();
    std::string getCurrentTimestamp();
    
    std::string version;
    std::string crashDirectory;
    std::map<std::string, std::string> metadata;
    CrashCallback crashCallback;
};

} // namespace Debug
} // namespace JJM

#endif
