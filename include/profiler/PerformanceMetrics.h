#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace JJM {
namespace Performance {

struct PerformanceMetric {
    std::string name;
    double value;
    double minValue;
    double maxValue;
    double avgValue;
    size_t sampleCount;
    
    PerformanceMetric() : name(""), value(0.0), minValue(0.0), 
                         maxValue(0.0), avgValue(0.0), sampleCount(0) {}
};

class PerformanceCounter {
public:
    PerformanceCounter(const std::string& name);
    ~PerformanceCounter();
    
    void start();
    void stop();
    void reset();
    
    double getElapsedMilliseconds() const;
    double getElapsedMicroseconds() const;
    
    const std::string& getName() const { return name; }

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    bool running;
};

class FrameTimer {
public:
    FrameTimer();
    ~FrameTimer();
    
    void tick();
    
    double getDeltaTime() const { return deltaTime; }
    double getFPS() const { return fps; }
    
    double getAverageFPS() const { return averageFPS; }
    double getMinFPS() const { return minFPS; }
    double getMaxFPS() const { return maxFPS; }
    
    void reset();

private:
    std::chrono::high_resolution_clock::time_point lastFrameTime;
    double deltaTime;
    double fps;
    double averageFPS;
    double minFPS;
    double maxFPS;
    
    std::vector<double> fpsHistory;
    size_t maxSamples;
    
    void updateFPSStats();
};

class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();
    
    static PerformanceProfiler& getInstance();
    
    void beginSample(const std::string& name);
    void endSample(const std::string& name);
    
    PerformanceMetric* getMetric(const std::string& name);
    
    void update();
    void reset();
    void clear();
    
    const std::unordered_map<std::string, PerformanceMetric>& getMetrics() const {
        return metrics;
    }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    std::unordered_map<std::string, PerformanceMetric> metrics;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> activeSamples;
    bool enabled;
    
    void recordSample(const std::string& name, double value);
};

class ScopedPerformanceTimer {
public:
    ScopedPerformanceTimer(const std::string& name);
    ~ScopedPerformanceTimer();

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
};

class MemoryTracker {
public:
    MemoryTracker();
    ~MemoryTracker();
    
    static MemoryTracker& getInstance();
    
    void trackAllocation(size_t size);
    void trackDeallocation(size_t size);
    
    size_t getTotalAllocated() const { return totalAllocated; }
    size_t getCurrentUsage() const { return currentUsage; }
    size_t getPeakUsage() const { return peakUsage; }
    
    size_t getAllocationCount() const { return allocationCount; }
    size_t getDeallocationCount() const { return deallocationCount; }
    
    void reset();

private:
    size_t totalAllocated;
    size_t currentUsage;
    size_t peakUsage;
    size_t allocationCount;
    size_t deallocationCount;
};

class PerformanceStats {
public:
    PerformanceStats();
    ~PerformanceStats();
    
    void update(double deltaTime);
    void reset();
    
    void incrementDrawCalls() { drawCalls++; }
    void incrementTriangles(size_t count) { triangles += count; }
    void incrementVertices(size_t count) { vertices += count; }
    
    void setDrawCalls(size_t count) { drawCalls = count; }
    void setTriangles(size_t count) { triangles = count; }
    void setVertices(size_t count) { vertices = count; }
    
    size_t getDrawCalls() const { return drawCalls; }
    size_t getTriangles() const { return triangles; }
    size_t getVertices() const { return vertices; }
    
    double getFrameTime() const { return frameTime; }
    double getFPS() const { return fps; }

private:
    size_t drawCalls;
    size_t triangles;
    size_t vertices;
    double frameTime;
    double fps;
};

class PerformanceGraph {
public:
    PerformanceGraph(size_t maxSamples = 100);
    ~PerformanceGraph();
    
    void addSample(double value);
    void clear();
    
    double getAverage() const;
    double getMin() const;
    double getMax() const;
    
    const std::vector<double>& getSamples() const { return samples; }
    
    void render();

private:
    std::vector<double> samples;
    size_t maxSamples;
    size_t currentIndex;
};

#define PROFILE_SCOPE(name) JJM::Performance::ScopedPerformanceTimer __profiler##__LINE__(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)

} // namespace Performance
} // namespace JJM
