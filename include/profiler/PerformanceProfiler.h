#ifndef PERFORMANCE_METRICS_H
#define PERFORMANCE_METRICS_H

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <fstream>

namespace JJM {
namespace Utils {

// Forward declarations
class ProfilerSession;
class ScopedTimer;

// =============================================================================
// Performance Metrics Core
// =============================================================================

struct FrameStats {
    float fps = 0.0f;
    float frameTime = 0.0f;        // ms
    float avgFrameTime = 0.0f;
    float minFrameTime = 0.0f;
    float maxFrameTime = 0.0f;
    uint64_t frameCount = 0;
    
    // Advanced metrics
    float variance = 0.0f;
    float percentile95 = 0.0f;
    float percentile99 = 0.0f;
};

struct MemoryStats {
    size_t totalAllocated = 0;     // bytes
    size_t totalFreed = 0;
    size_t currentUsage = 0;
    size_t peakUsage = 0;
    size_t allocationCount = 0;
    size_t deallocationCount = 0;
    
    // Breakdown by category
    std::unordered_map<std::string, size_t> categoryUsage;
};

struct CPUStats {
    float totalTime = 0.0f;        // ms
    float systemTime = 0.0f;
    float userTime = 0.0f;
    float idleTime = 0.0f;
    float cpuUsage = 0.0f;         // percentage
    int threadCount = 0;
};

struct GPUStats {
    float drawCallCount = 0;
    float triangleCount = 0;
    float textureMemory = 0;       // MB
    float bufferMemory = 0;
    float frameBufferMemory = 0;
    float gpuTime = 0.0f;          // ms
};

struct ProfileEntry {
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    double duration = 0.0;         // microseconds
    int depth = 0;
    int threadId = 0;
    bool completed = false;
};

// =============================================================================
// Performance Profiler
// =============================================================================

class PerformanceProfiler {
private:
    static PerformanceProfiler* instance;
    
    // Frame timing
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    std::vector<float> frameTimeHistory;
    size_t maxHistorySize = 120;   // 2 seconds at 60fps
    
    // Statistics
    FrameStats frameStats;
    MemoryStats memoryStats;
    CPUStats cpuStats;
    GPUStats gpuStats;
    
    // Profiling
    std::vector<ProfileEntry> profileEntries;
    std::vector<ProfileEntry> completedEntries;
    std::unordered_map<std::string, std::vector<double>> timerHistory;
    int currentDepth = 0;
    
    // Threading
    mutable std::mutex statsMutex;
    mutable std::mutex profileMutex;
    
    // Configuration
    bool profilingEnabled = true;
    bool memoryTrackingEnabled = true;
    bool detailedLogging = false;
    
    // Session management
    std::unique_ptr<ProfilerSession> activeSession;
    
    PerformanceProfiler();

public:
    ~PerformanceProfiler();
    
    static PerformanceProfiler* getInstance();
    static void cleanup();
    
    // Frame timing
    void beginFrame();
    void endFrame();
    const FrameStats& getFrameStats() const { return frameStats; }
    
    // Memory tracking
    void trackAllocation(size_t size, const std::string& category = "default");
    void trackDeallocation(size_t size, const std::string& category = "default");
    const MemoryStats& getMemoryStats() const { return memoryStats; }
    void resetMemoryStats();
    
    // GPU tracking
    void trackDrawCall(int triangles = 0);
    void setTextureMemory(float mb) { gpuStats.textureMemory = mb; }
    void setBufferMemory(float mb) { gpuStats.bufferMemory = mb; }
    const GPUStats& getGPUStats() const { return gpuStats; }
    
    // Profiling
    void beginProfile(const std::string& name);
    void endProfile(const std::string& name);
    
    // Timer history
    double getAverageTime(const std::string& name) const;
    double getMinTime(const std::string& name) const;
    double getMaxTime(const std::string& name) const;
    const std::vector<double>& getTimeHistory(const std::string& name) const;
    
    // Session management
    void beginSession(const std::string& name, const std::string& filepath = "");
    void endSession();
    bool isSessionActive() const { return activeSession != nullptr; }
    
    // Configuration
    void setProfilingEnabled(bool enabled) { profilingEnabled = enabled; }
    void setMemoryTrackingEnabled(bool enabled) { memoryTrackingEnabled = enabled; }
    void setDetailedLogging(bool enabled) { detailedLogging = enabled; }
    void setMaxHistorySize(size_t size) { maxHistorySize = size; }
    
    // Reporting
    std::string generateReport() const;
    void printReport() const;
    void exportToCSV(const std::string& filepath) const;
    void exportToJSON(const std::string& filepath) const;
    
    // Analysis
    std::vector<std::string> detectBottlenecks(float thresholdMs = 16.0f) const;
    float calculateMemoryFragmentation() const;
    
    // Utilities
    void clear();
    size_t getProfileEntryCount() const { return completedEntries.size(); }
    
private:
    void updateFrameStats(float deltaTime);
    void writeSessionData(const ProfileEntry& entry);
    std::string getCurrentThreadName() const;
};

// =============================================================================
// Scoped Timer - RAII-based profiling
// =============================================================================

class ScopedTimer {
private:
    std::string name;
    bool active;

public:
    explicit ScopedTimer(const std::string& timerName);
    ~ScopedTimer();
    
    // Disable copying
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;
};

// =============================================================================
// Profiler Session - Chrome Tracing Format
// =============================================================================

class ProfilerSession {
private:
    std::string name;
    std::string filepath;
    std::ofstream outputStream;
    int profileCount = 0;
    
    std::chrono::high_resolution_clock::time_point sessionStart;

public:
    ProfilerSession(const std::string& sessionName, const std::string& file);
    ~ProfilerSession();
    
    void writeHeader();
    void writeFooter();
    void writeProfile(const ProfileEntry& entry);
    
    std::string getName() const { return name; }
    bool isOpen() const { return outputStream.is_open(); }
};

// =============================================================================
// Memory Allocator Wrapper (Optional tracking)
// =============================================================================

template<typename T>
class TrackedAllocator {
private:
    std::string category;

public:
    using value_type = T;
    
    TrackedAllocator(const std::string& cat = "default") : category(cat) {}
    
    template<typename U>
    TrackedAllocator(const TrackedAllocator<U>& other) : category(other.category) {}
    
    T* allocate(size_t n) {
        size_t bytes = n * sizeof(T);
        PerformanceProfiler::getInstance()->trackAllocation(bytes, category);
        return static_cast<T*>(::operator new(bytes));
    }
    
    void deallocate(T* p, size_t n) {
        size_t bytes = n * sizeof(T);
        PerformanceProfiler::getInstance()->trackDeallocation(bytes, category);
        ::operator delete(p);
    }
    
    template<typename U>
    friend class TrackedAllocator;
};

// =============================================================================
// Performance Budget System
// =============================================================================

struct PerformanceBudget {
    float frameTimeBudget = 16.67f;    // 60 FPS
    float renderTimeBudget = 10.0f;
    float updateTimeBudget = 5.0f;
    float physicsTimeBudget = 3.0f;
    float aiTimeBudget = 2.0f;
    
    size_t memoryBudget = 512 * 1024 * 1024;  // 512 MB
    size_t textureBudget = 128 * 1024 * 1024;  // 128 MB
    size_t meshBudget = 64 * 1024 * 1024;      // 64 MB
    
    int drawCallBudget = 1000;
    int triangleBudget = 500000;
};

class BudgetMonitor {
private:
    PerformanceBudget budget;
    std::unordered_map<std::string, float> categoryTimes;
    std::vector<std::string> budgetViolations;

public:
    void setBudget(const PerformanceBudget& newBudget) { budget = newBudget; }
    const PerformanceBudget& getBudget() const { return budget; }
    
    void checkFrameTimeBudget(float frameTime);
    void checkMemoryBudget(size_t usage);
    void checkDrawCallBudget(int drawCalls);
    
    bool isWithinBudget() const { return budgetViolations.empty(); }
    const std::vector<std::string>& getViolations() const { return budgetViolations; }
    void clearViolations() { budgetViolations.clear(); }
};

// =============================================================================
// Frame Analyzer - Advanced frame timing analysis
// =============================================================================

class FrameAnalyzer {
private:
    std::vector<float> frameTimes;
    size_t maxSamples = 300;  // 5 seconds at 60fps

public:
    void addFrame(float frameTime);
    
    // Statistical analysis
    float getMean() const;
    float getMedian() const;
    float getStandardDeviation() const;
    float getPercentile(float p) const;
    
    // Spike detection
    std::vector<int> detectSpikes(float threshold = 2.0f) const;
    int getSpikeCount(float threshold = 2.0f) const;
    
    // Frame pacing
    bool isFramePacingStable(float tolerance = 0.1f) const;
    float getFramePacingVariability() const;
    
    // Reporting
    std::string generateHistogram(int bins = 10) const;
    
    void clear() { frameTimes.clear(); }
    size_t getSampleCount() const { return frameTimes.size(); }
};

// =============================================================================
// Call Stack Profiler
// =============================================================================

struct CallStackEntry {
    std::string functionName;
    double inclusiveTime = 0.0;  // Including child calls
    double exclusiveTime = 0.0;  // Excluding child calls
    int callCount = 0;
    std::vector<CallStackEntry> children;
};

class CallStackProfiler {
private:
    CallStackEntry root;
    std::vector<CallStackEntry*> stack;

public:
    void push(const std::string& name);
    void pop();
    
    const CallStackEntry& getRoot() const { return root; }
    void reset();
    
    // Analysis
    std::vector<CallStackEntry> getFlattenedProfile() const;
    std::string generateFlameGraph() const;
};

// =============================================================================
// Macros for convenient profiling
// =============================================================================

#ifdef PROFILING_ENABLED
    #define PROFILE_FUNCTION() ScopedTimer timer##__LINE__(__FUNCTION__)
    #define PROFILE_SCOPE(name) ScopedTimer timer##__LINE__(name)
    #define PROFILE_BEGIN(name) PerformanceProfiler::getInstance()->beginProfile(name)
    #define PROFILE_END(name) PerformanceProfiler::getInstance()->endProfile(name)
#else
    #define PROFILE_FUNCTION()
    #define PROFILE_SCOPE(name)
    #define PROFILE_BEGIN(name)
    #define PROFILE_END(name)
#endif

} // namespace Utils
} // namespace JJM

#endif // PERFORMANCE_METRICS_H
