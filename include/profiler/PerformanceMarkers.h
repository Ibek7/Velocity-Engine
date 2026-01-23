#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <unordered_map>

/**
 * @file PerformanceMarkers.h
 * @brief Performance profiling markers and helpers
 * 
 * Provides lightweight performance markers for profiling specific
 * code sections and generating detailed timing reports.
 */

namespace Engine {

/**
 * @class PerformanceMarker
 * @brief Single performance measurement marker
 */
class PerformanceMarker {
public:
    PerformanceMarker(const std::string& name);
    ~PerformanceMarker();
    
    /**
     * @brief Get marker name
     * @return Marker name
     */
    const std::string& getName() const { return m_name; }
    
    /**
     * @brief Get elapsed time in milliseconds
     * @return Elapsed time
     */
    double getElapsedMs() const;
    
    /**
     * @brief End marker manually
     */
    void end();

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_endTime;
    bool m_ended;
};

/**
 * @class ScopedPerformanceMarker
 * @brief RAII-style performance marker
 */
class ScopedPerformanceMarker {
public:
    ScopedPerformanceMarker(const std::string& name);
    ~ScopedPerformanceMarker();

private:
    std::string m_name;
    std::chrono::high_resolution_clock::time_point m_startTime;
};

/**
 * @struct PerformanceStats
 * @brief Statistics for a performance marker
 */
struct PerformanceStats {
    std::string name;
    int callCount;
    double totalTime;
    double minTime;
    double maxTime;
    double avgTime;
    
    PerformanceStats()
        : callCount(0)
        , totalTime(0.0)
        , minTime(std::numeric_limits<double>::max())
        , maxTime(0.0)
        , avgTime(0.0) {}
};

/**
 * @class PerformanceProfiler
 * @brief Collects and analyzes performance markers
 */
class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance();
    
    /**
     * @brief Begin a profiling section
     * @param name Section name
     */
    void beginSection(const std::string& name);
    
    /**
     * @brief End a profiling section
     * @param name Section name
     */
    void endSection(const std::string& name);
    
    /**
     * @brief Record a measurement
     * @param name Measurement name
     * @param timeMs Time in milliseconds
     */
    void recordMeasurement(const std::string& name, double timeMs);
    
    /**
     * @brief Get statistics for a section
     * @param name Section name
     * @return Performance statistics
     */
    PerformanceStats getStats(const std::string& name) const;
    
    /**
     * @brief Get all statistics
     * @return Vector of all statistics
     */
    std::vector<PerformanceStats> getAllStats() const;
    
    /**
     * @brief Clear all measurements
     */
    void clear();
    
    /**
     * @brief Reset statistics for a section
     * @param name Section name
     */
    void resetSection(const std::string& name);
    
    /**
     * @brief Enable/disable profiling
     * @param enabled Enable flag
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * @brief Check if profiling is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Generate performance report
     * @return Report string
     */
    std::string generateReport() const;
    
    /**
     * @brief Export statistics to CSV
     * @param filename Output file path
     * @return True if successful
     */
    bool exportToCSV(const std::string& filename) const;
    
    /**
     * @brief Set frame marker (for per-frame profiling)
     */
    void markFrame();
    
    /**
     * @brief Get average frame time
     * @return Average frame time in ms
     */
    double getAverageFrameTime() const;
    
    /**
     * @brief Get current FPS
     * @return Frames per second
     */
    double getCurrentFPS() const;

private:
    PerformanceProfiler();
    ~PerformanceProfiler() = default;
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;
    
    struct SectionData {
        std::string name;
        std::chrono::high_resolution_clock::time_point startTime;
        std::vector<double> measurements;
        bool isActive;
        
        SectionData() : isActive(false) {}
    };
    
    std::unordered_map<std::string, SectionData> m_sections;
    std::vector<double> m_frameTimes;
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;
    bool m_enabled;
    int m_maxFrameSamples;
};

/**
 * @class GPUPerformanceMarker
 * @brief GPU-side performance markers
 */
class GPUPerformanceMarker {
public:
    GPUPerformanceMarker(const std::string& name);
    ~GPUPerformanceMarker();
    
    /**
     * @brief Begin GPU timing
     */
    void begin();
    
    /**
     * @brief End GPU timing
     */
    void end();
    
    /**
     * @brief Get elapsed GPU time
     * @return Time in milliseconds
     */
    double getElapsedMs() const;
    
    /**
     * @brief Check if results are available
     * @return True if results ready
     */
    bool isReady() const;

private:
    std::string m_name;
    unsigned int m_queryStart;
    unsigned int m_queryEnd;
    bool m_active;
};

/**
 * @class PerformanceBudget
 * @brief Tracks performance against budgets
 */
class PerformanceBudget {
public:
    /**
     * @brief Set budget for a section
     * @param name Section name
     * @param budgetMs Budget in milliseconds
     */
    void setBudget(const std::string& name, double budgetMs);
    
    /**
     * @brief Check if section is over budget
     * @param name Section name
     * @param actualTimeMs Actual time taken
     * @return True if over budget
     */
    bool isOverBudget(const std::string& name, double actualTimeMs) const;
    
    /**
     * @brief Get budget utilization percentage
     * @param name Section name
     * @param actualTimeMs Actual time taken
     * @return Utilization percentage (100 = at budget)
     */
    double getBudgetUtilization(const std::string& name, double actualTimeMs) const;
    
    /**
     * @brief Get all sections over budget
     * @return Vector of section names
     */
    std::vector<std::string> getSectionsOverBudget() const;
    
    /**
     * @brief Generate budget report
     * @return Report string
     */
    std::string generateBudgetReport() const;

private:
    std::unordered_map<std::string, double> m_budgets;
};

/**
 * @brief Macro for easy scoped profiling
 */
#define PROFILE_SCOPE(name) Engine::ScopedPerformanceMarker _profileMarker##__LINE__(name)

/**
 * @brief Macro for function profiling
 */
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace Engine
