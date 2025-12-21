#include "debug/CrashReporter.h"
#include <fstream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <iostream>

namespace JJM {
namespace Debug {

CrashReporter& CrashReporter::getInstance() {
    static CrashReporter instance;
    return instance;
}

CrashReporter::CrashReporter() 
    : version("1.0.0"), crashDirectory("crashes/") {
}

CrashReporter::~CrashReporter() {
}

void CrashReporter::initialize() {
    std::cout << "Crash reporter initialized" << std::endl;
}

void CrashReporter::shutdown() {
    std::cout << "Crash reporter shutdown" << std::endl;
}

void CrashReporter::reportCrash(const std::string& message) {
    reportCrash(message, generateStackTrace());
}

void CrashReporter::reportCrash(const std::string& message, const std::string& stackTrace) {
    CrashInfo info;
    info.message = message;
    info.stackTrace = stackTrace;
    info.timestamp = getCurrentTimestamp();
    info.version = version;
    info.metadata = metadata;
    
    writeCrashReport(info);
    
    if (crashCallback) {
        crashCallback(info);
    }
}

void CrashReporter::addMetadata(const std::string& key, const std::string& value) {
    metadata[key] = value;
}

void CrashReporter::setVersion(const std::string& ver) {
    version = ver;
}

void CrashReporter::setCrashDirectory(const std::string& dir) {
    crashDirectory = dir;
}

void CrashReporter::setCrashCallback(CrashCallback callback) {
    crashCallback = callback;
}

std::vector<CrashInfo> CrashReporter::getRecentCrashes() const {
    return std::vector<CrashInfo>();
}

void CrashReporter::writeCrashReport(const CrashInfo& info) {
    std::string filename = crashDirectory + "crash_" + info.timestamp + ".txt";
    
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "CRASH REPORT\n";
        file << "=============\n\n";
        file << "Time: " << info.timestamp << "\n";
        file << "Version: " << info.version << "\n";
        file << "Message: " << info.message << "\n\n";
        
        if (!info.metadata.empty()) {
            file << "Metadata:\n";
            for (const auto& pair : info.metadata) {
                file << "  " << pair.first << ": " << pair.second << "\n";
            }
            file << "\n";
        }
        
        file << "Stack Trace:\n";
        file << info.stackTrace << "\n";
        
        file.close();
        std::cout << "Crash report written to: " << filename << std::endl;
    }
}

std::string CrashReporter::generateStackTrace() {
    return "[Stack trace placeholder]";
}

std::string CrashReporter::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* timeinfo = std::localtime(&now);
    
    std::ostringstream oss;
    oss << std::put_time(timeinfo, "%Y%m%d_%H%M%S");
    
    return oss.str();
}

} // namespace Debug
} // namespace JJM
