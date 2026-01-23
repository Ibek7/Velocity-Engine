#include "profiler/PerformanceMarkers.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iomanip>

namespace Engine {

// PerformanceMarker Implementation
PerformanceMarker::PerformanceMarker(const std::string& name)
    : m_name(name)
    , m_startTime(std::chrono::high_resolution_clock::now())
    , m_ended(false) {
}

PerformanceMarker::~PerformanceMarker() {
    if (!m_ended) {
        end();
    }
}

double PerformanceMarker::getElapsedMs() const {
    auto endTime = m_ended ? m_endTime : std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
    return duration.count() / 1000.0;
}

void PerformanceMarker::end() {
    if (!m_ended) {
        m_endTime = std::chrono::high_resolution_clock::now();
        m_ended = true;
        
        PerformanceProfiler::getInstance().recordMeasurement(m_name, getElapsedMs());
    }
}

// ScopedPerformanceMarker Implementation
ScopedPerformanceMarker::ScopedPerformanceMarker(const std::string& name)
    : m_name(name)
    , m_startTime(std::chrono::high_resolution_clock::now()) {
    PerformanceProfiler::getInstance().beginSection(name);
}

ScopedPerformanceMarker::~ScopedPerformanceMarker() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_startTime);
    double elapsedMs = duration.count() / 1000.0;
    
    PerformanceProfiler::getInstance().endSection(m_name);
    PerformanceProfiler::getInstance().recordMeasurement(m_name, elapsedMs);
}

// PerformanceProfiler Implementation
PerformanceProfiler::PerformanceProfiler()
    : m_enabled(true)
    , m_maxFrameSamples(100)
    , m_lastFrameTime(std::chrono::high_resolution_clock::now()) {
    m_frameTimes.reserve(m_maxFrameSamples);
}

PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

void PerformanceProfiler::beginSection(const std::string& name) {
    if (!m_enabled) return;
    
    auto& section = m_sections[name];
    section.name = name;
    section.startTime = std::chrono::high_resolution_clock::now();
    section.isActive = true;
}

void PerformanceProfiler::endSection(const std::string& name) {
    if (!m_enabled) return;
    
    auto it = m_sections.find(name);
    if (it != m_sections.end() && it->second.isActive) {
        it->second.isActive = false;
    }
}

void PerformanceProfiler::recordMeasurement(const std::string& name, double timeMs) {
    if (!m_enabled) return;
    
    auto& section = m_sections[name];
    section.name = name;
    section.measurements.push_back(timeMs);
}

PerformanceStats PerformanceProfiler::getStats(const std::string& name) const {
    PerformanceStats stats;
    stats.name = name;
    
    auto it = m_sections.find(name);
    if (it == m_sections.end() || it->second.measurements.empty()) {
        return stats;
    }
    
    const auto& measurements = it->second.measurements;
    stats.callCount = static_cast<int>(measurements.size());
    stats.totalTime = 0.0;
    
    for (double time : measurements) {
        stats.totalTime += time;
        stats.minTime = std::min(stats.minTime, time);
        stats.maxTime = std::max(stats.maxTime, time);
    }
    
    stats.avgTime = stats.totalTime / stats.callCount;
    
    return stats;
}

std::vector<PerformanceStats> PerformanceProfiler::getAllStats() const {
    std::vector<PerformanceStats> allStats;
    allStats.reserve(m_sections.size());
    
    for (const auto& pair : m_sections) {
        allStats.push_back(getStats(pair.first));
    }
    
    // Sort by total time descending
    std::sort(allStats.begin(), allStats.end(),
        [](const PerformanceStats& a, const PerformanceStats& b) {
            return a.totalTime > b.totalTime;
        });
    
    return allStats;
}

void PerformanceProfiler::clear() {
    m_sections.clear();
    m_frameTimes.clear();
}

void PerformanceProfiler::resetSection(const std::string& name) {
    auto it = m_sections.find(name);
    if (it != m_sections.end()) {
        it->second.measurements.clear();
    }
}

std::string PerformanceProfiler::generateReport() const {
    std::ostringstream oss;
    oss << "\n=== Performance Report ===\n\n";
    oss << std::fixed << std::setprecision(3);
    
    auto stats = getAllStats();
    
    if (stats.empty()) {
        oss << "No performance data collected.\n";
        return oss.str();
    }
    
    // Header
    oss << std::left << std::setw(30) << "Section"
        << std::right << std::setw(10) << "Calls"
        << std::setw(12) << "Total (ms)"
        << std::setw(12) << "Avg (ms)"
        << std::setw(12) << "Min (ms)"
        << std::setw(12) << "Max (ms)" << "\n";
    oss << std::string(88, '-') << "\n";
    
    // Data rows
    for (const auto& stat : stats) {
        oss << std::left << std::setw(30) << stat.name
            << std::right << std::setw(10) << stat.callCount
            << std::setw(12) << stat.totalTime
            << std::setw(12) << stat.avgTime
            << std::setw(12) << stat.minTime
            << std::setw(12) << stat.maxTime << "\n";
    }
    
    // Frame timing if available
    if (!m_frameTimes.empty()) {
        oss << "\n=== Frame Timing ===\n";
        oss << "Average Frame Time: " << getAverageFrameTime() << " ms\n";
        oss << "Average FPS: " << getCurrentFPS() << "\n";
    }
    
    return oss.str();
}

bool PerformanceProfiler::exportToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    // Header
    file << "Section,Calls,Total(ms),Average(ms),Min(ms),Max(ms)\n";
    
    // Data
    auto stats = getAllStats();
    for (const auto& stat : stats) {
        file << stat.name << ","
             << stat.callCount << ","
             << stat.totalTime << ","
             << stat.avgTime << ","
             << stat.minTime << ","
             << stat.maxTime << "\n";
    }
    
    file.close();
    return true;
}

void PerformanceProfiler::markFrame() {
    if (!m_enabled) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - m_lastFrameTime);
    double frameTimeMs = duration.count() / 1000.0;
    
    if (m_frameTimes.size() >= static_cast<size_t>(m_maxFrameSamples)) {
        m_frameTimes.erase(m_frameTimes.begin());
    }
    m_frameTimes.push_back(frameTimeMs);
    
    m_lastFrameTime = now;
}

double PerformanceProfiler::getAverageFrameTime() const {
    if (m_frameTimes.empty()) {
        return 0.0;
    }
    
    double total = 0.0;
    for (double time : m_frameTimes) {
        total += time;
    }
    
    return total / m_frameTimes.size();
}

double PerformanceProfiler::getCurrentFPS() const {
    double avgFrameTime = getAverageFrameTime();
    return avgFrameTime > 0.0 ? 1000.0 / avgFrameTime : 0.0;
}

// GPUPerformanceMarker Implementation
GPUPerformanceMarker::GPUPerformanceMarker(const std::string& name)
    : m_name(name)
    , m_queryStart(0)
    , m_queryEnd(0)
    , m_active(false) {
}

GPUPerformanceMarker::~GPUPerformanceMarker() {
    // In a real implementation, clean up GPU queries
}

void GPUPerformanceMarker::begin() {
    // In a real implementation, create and start GPU query
    m_active = true;
}

void GPUPerformanceMarker::end() {
    // In a real implementation, end GPU query
    m_active = false;
}

double GPUPerformanceMarker::getElapsedMs() const {
    // In a real implementation, retrieve GPU query results
    return 0.0;
}

bool GPUPerformanceMarker::isReady() const {
    // In a real implementation, check if GPU query results are available
    return true;
}

// PerformanceBudget Implementation
void PerformanceBudget::setBudget(const std::string& name, double budgetMs) {
    m_budgets[name] = budgetMs;
}

bool PerformanceBudget::isOverBudget(const std::string& name, double actualTimeMs) const {
    auto it = m_budgets.find(name);
    if (it == m_budgets.end()) {
        return false;
    }
    return actualTimeMs > it->second;
}

double PerformanceBudget::getBudgetUtilization(const std::string& name, double actualTimeMs) const {
    auto it = m_budgets.find(name);
    if (it == m_budgets.end() || it->second == 0.0) {
        return 0.0;
    }
    return (actualTimeMs / it->second) * 100.0;
}

std::vector<std::string> PerformanceBudget::getSectionsOverBudget() const {
    std::vector<std::string> overBudget;
    
    auto& profiler = PerformanceProfiler::getInstance();
    for (const auto& pair : m_budgets) {
        auto stats = profiler.getStats(pair.first);
        if (stats.avgTime > pair.second) {
            overBudget.push_back(pair.first);
        }
    }
    
    return overBudget;
}

std::string PerformanceBudget::generateBudgetReport() const {
    std::ostringstream oss;
    oss << "\n=== Performance Budget Report ===\n\n";
    oss << std::fixed << std::setprecision(2);
    
    auto& profiler = PerformanceProfiler::getInstance();
    
    for (const auto& pair : m_budgets) {
        auto stats = profiler.getStats(pair.first);
        double utilization = getBudgetUtilization(pair.first, stats.avgTime);
        
        oss << pair.first << ":\n";
        oss << "  Budget: " << pair.second << " ms\n";
        oss << "  Actual: " << stats.avgTime << " ms\n";
        oss << "  Utilization: " << utilization << "%\n";
        
        if (utilization > 100.0) {
            oss << "  [WARNING: OVER BUDGET]\n";
        }
        oss << "\n";
    }
    
    return oss.str();
}

} // namespace Engine
