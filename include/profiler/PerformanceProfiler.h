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

// =============================================================================
// GPU Profiling - Timing Queries and Pipeline Statistics
// =============================================================================

/**
 * @brief GPU pipeline stages for profiling
 */
enum class GPUPipelineStage {
    VertexInput,
    VertexShader,
    TessellationControl,
    TessellationEval,
    GeometryShader,
    FragmentShader,
    ComputeShader,
    Transfer,
    Present,
    RayTracing
};

/**
 * @brief GPU query types
 */
enum class GPUQueryType {
    Timestamp,
    PipelineStatistics,
    Occlusion,
    PrimitivesGenerated,
    TransformFeedback,
    BinaryOcclusion
};

/**
 * @brief GPU timing query result
 */
struct GPUTimingResult {
    std::string name;
    uint64_t startTimestamp;
    uint64_t endTimestamp;
    double durationMs;
    GPUPipelineStage stage;
    int frameNumber;
    bool valid;
    
    GPUTimingResult()
        : startTimestamp(0)
        , endTimestamp(0)
        , durationMs(0.0)
        , stage(GPUPipelineStage::VertexShader)
        , frameNumber(0)
        , valid(false)
    {}
};

/**
 * @brief GPU pipeline statistics
 */
struct GPUPipelineStats {
    uint64_t inputAssemblyVertices;
    uint64_t inputAssemblyPrimitives;
    uint64_t vertexShaderInvocations;
    uint64_t geometryShaderInvocations;
    uint64_t geometryShaderPrimitives;
    uint64_t clippingInvocations;
    uint64_t clippingPrimitives;
    uint64_t fragmentShaderInvocations;
    uint64_t tessControlPatches;
    uint64_t tessEvalShaderInvocations;
    uint64_t computeShaderInvocations;
    
    GPUPipelineStats()
        : inputAssemblyVertices(0)
        , inputAssemblyPrimitives(0)
        , vertexShaderInvocations(0)
        , geometryShaderInvocations(0)
        , geometryShaderPrimitives(0)
        , clippingInvocations(0)
        , clippingPrimitives(0)
        , fragmentShaderInvocations(0)
        , tessControlPatches(0)
        , tessEvalShaderInvocations(0)
        , computeShaderInvocations(0)
    {}
};

/**
 * @brief GPU memory pool info
 */
struct GPUMemoryPool {
    std::string name;
    size_t totalSize;
    size_t usedSize;
    size_t peakUsage;
    size_t allocationCount;
    bool deviceLocal;
    bool hostVisible;
    
    GPUMemoryPool()
        : totalSize(0)
        , usedSize(0)
        , peakUsage(0)
        , allocationCount(0)
        , deviceLocal(false)
        , hostVisible(false)
    {}
    
    float getUsagePercent() const {
        return totalSize > 0 ? (static_cast<float>(usedSize) / totalSize) * 100.0f : 0.0f;
    }
};

/**
 * @brief GPU resource tracking entry
 */
struct GPUResourceInfo {
    std::string name;
    std::string type;       // "Texture", "Buffer", "RenderTarget", "Pipeline"
    size_t sizeBytes;
    int width;
    int height;
    int mipLevels;
    std::string format;
    bool inUse;
    int referenceCount;
    std::chrono::steady_clock::time_point lastAccess;
    
    GPUResourceInfo()
        : sizeBytes(0)
        , width(0)
        , height(0)
        , mipLevels(1)
        , inUse(true)
        , referenceCount(0)
    {}
};

/**
 * @brief GPU query pool for managing timing queries
 */
class GPUQueryPool {
private:
    struct QuerySlot {
        uint32_t queryId;
        std::string name;
        GPUQueryType type;
        bool inUse;
        bool resultReady;
        uint64_t result;
    };
    
    std::vector<QuerySlot> queries;
    uint32_t poolSize;
    uint32_t nextFreeSlot;
    uint64_t timestampPeriod;   // nanoseconds per tick
    
public:
    GPUQueryPool(uint32_t size = 256);
    
    uint32_t allocateQuery(GPUQueryType type, const std::string& name);
    void releaseQuery(uint32_t slot);
    void reset();
    
    bool isResultReady(uint32_t slot) const;
    uint64_t getResult(uint32_t slot) const;
    
    void setTimestampPeriod(uint64_t period) { timestampPeriod = period; }
    double ticksToMs(uint64_t ticks) const;
    
    size_t getActiveQueryCount() const;
    size_t getPoolSize() const { return poolSize; }
};

/**
 * @brief GPU profiler for graphics pipeline analysis
 */
class GPUProfiler {
private:
    static GPUProfiler* instance;
    
    // Query management
    std::unique_ptr<GPUQueryPool> queryPool;
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> activeTimers; // start, end query IDs
    
    // Results
    std::vector<GPUTimingResult> frameResults;
    std::vector<GPUTimingResult> historyResults;
    std::unordered_map<std::string, std::vector<double>> timerHistory;
    
    // Pipeline statistics
    GPUPipelineStats currentStats;
    GPUPipelineStats frameStats;
    bool pipelineStatsEnabled;
    
    // Memory tracking
    std::vector<GPUMemoryPool> memoryPools;
    std::unordered_map<std::string, GPUResourceInfo> trackedResources;
    size_t totalGPUMemory;
    size_t usedGPUMemory;
    size_t peakGPUMemory;
    
    // Frame timing
    int currentFrame;
    double lastFrameGPUTime;
    std::vector<double> frameGPUTimes;
    
    // Configuration
    bool enabled;
    bool calibrated;
    int frameLatency;           // Frames to wait for results
    size_t maxHistorySize;
    
    mutable std::mutex profilerMutex;
    
    GPUProfiler();
    
public:
    ~GPUProfiler();
    
    static GPUProfiler* getInstance();
    static void cleanup();
    
    // Initialization
    void initialize(uint32_t queryPoolSize = 512);
    void calibrate();
    bool isCalibrated() const { return calibrated; }
    
    // Frame management
    void beginFrame();
    void endFrame();
    int getCurrentFrame() const { return currentFrame; }
    
    // Timing queries
    void beginTimer(const std::string& name, GPUPipelineStage stage = GPUPipelineStage::FragmentShader);
    void endTimer(const std::string& name);
    
    // Get results (may be from previous frames due to GPU latency)
    double getTimerResult(const std::string& name) const;
    double getAverageTime(const std::string& name) const;
    const std::vector<GPUTimingResult>& getFrameResults() const { return frameResults; }
    
    // Pipeline statistics
    void enablePipelineStats(bool enable) { pipelineStatsEnabled = enable; }
    void beginPipelineStatsQuery();
    void endPipelineStatsQuery();
    const GPUPipelineStats& getPipelineStats() const { return currentStats; }
    
    // Memory tracking
    void registerMemoryPool(const std::string& name, size_t totalSize, bool deviceLocal, bool hostVisible);
    void updatePoolUsage(const std::string& name, size_t used, size_t allocations);
    const std::vector<GPUMemoryPool>& getMemoryPools() const { return memoryPools; }
    
    void trackResource(const std::string& name, const GPUResourceInfo& info);
    void untrackResource(const std::string& name);
    void updateResourceAccess(const std::string& name);
    const std::unordered_map<std::string, GPUResourceInfo>& getTrackedResources() const { return trackedResources; }
    
    size_t getTotalGPUMemory() const { return totalGPUMemory; }
    size_t getUsedGPUMemory() const { return usedGPUMemory; }
    size_t getPeakGPUMemory() const { return peakGPUMemory; }
    
    // Frame GPU time
    double getLastFrameGPUTime() const { return lastFrameGPUTime; }
    double getAverageFrameGPUTime() const;
    
    // Analysis
    std::vector<std::string> findBottlenecks(double thresholdMs = 2.0) const;
    std::string getStageWithMostTime() const;
    
    // Reporting
    std::string generateReport() const;
    void exportToChrome(const std::string& filepath) const;
    
    // Configuration
    void setEnabled(bool enable) { enabled = enable; }
    bool isEnabled() const { return enabled; }
    void setFrameLatency(int latency) { frameLatency = latency; }
    void setMaxHistorySize(size_t size) { maxHistorySize = size; }
    
    // Clear/reset
    void clear();
    void resetStats();
    
private:
    void collectResults();
    void updateHistory(const std::string& name, double timeMs);
};

/**
 * @brief RAII GPU timer scope
 */
class ScopedGPUTimer {
private:
    std::string name;
    bool active;
    
public:
    ScopedGPUTimer(const std::string& timerName, GPUPipelineStage stage = GPUPipelineStage::FragmentShader);
    ~ScopedGPUTimer();
    
    ScopedGPUTimer(const ScopedGPUTimer&) = delete;
    ScopedGPUTimer& operator=(const ScopedGPUTimer&) = delete;
};

/**
 * @brief GPU marker for debug regions (shows in graphics debuggers)
 */
class GPUDebugMarker {
public:
    static void begin(const std::string& name, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    static void end();
    static void insert(const std::string& text);
};

/**
 * @brief Scoped GPU debug region
 */
class ScopedGPUDebugRegion {
private:
    std::string name;
    
public:
    ScopedGPUDebugRegion(const std::string& regionName, float r = 1.0f, float g = 1.0f, float b = 1.0f);
    ~ScopedGPUDebugRegion();
    
    ScopedGPUDebugRegion(const ScopedGPUDebugRegion&) = delete;
    ScopedGPUDebugRegion& operator=(const ScopedGPUDebugRegion&) = delete;
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
    
    // GPU profiling macros
    #define GPU_PROFILE_SCOPE(name) ScopedGPUTimer gpuTimer##__LINE__(name)
    #define GPU_PROFILE_SCOPE_STAGE(name, stage) ScopedGPUTimer gpuTimer##__LINE__(name, stage)
    #define GPU_PROFILE_BEGIN(name) GPUProfiler::getInstance()->beginTimer(name)
    #define GPU_PROFILE_END(name) GPUProfiler::getInstance()->endTimer(name)
    #define GPU_DEBUG_REGION(name) ScopedGPUDebugRegion gpuRegion##__LINE__(name)
    #define GPU_DEBUG_REGION_COLOR(name, r, g, b) ScopedGPUDebugRegion gpuRegion##__LINE__(name, r, g, b)
    #define GPU_DEBUG_MARKER(text) GPUDebugMarker::insert(text)
#else
    #define PROFILE_FUNCTION()
    #define PROFILE_SCOPE(name)
    #define PROFILE_BEGIN(name)
    #define PROFILE_END(name)
    
    #define GPU_PROFILE_SCOPE(name)
    #define GPU_PROFILE_SCOPE_STAGE(name, stage)
    #define GPU_PROFILE_BEGIN(name)
    #define GPU_PROFILE_END(name)
    #define GPU_DEBUG_REGION(name)
    #define GPU_DEBUG_REGION_COLOR(name, r, g, b)
    #define GPU_DEBUG_MARKER(text)
#endif

} // namespace Utils
} // namespace JJM

#endif // PERFORMANCE_METRICS_H
