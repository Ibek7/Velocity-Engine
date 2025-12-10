#include "profiler/PerformanceMetrics.h"
#include <algorithm>
#include <numeric>
#include <iostream>

namespace JJM {
namespace Performance {

// PerformanceCounter implementation
PerformanceCounter::PerformanceCounter(const std::string& name)
    : name(name), running(false) {}

PerformanceCounter::~PerformanceCounter() {}

void PerformanceCounter::start() {
    startTime = std::chrono::high_resolution_clock::now();
    running = true;
}

void PerformanceCounter::stop() {
    endTime = std::chrono::high_resolution_clock::now();
    running = false;
}

void PerformanceCounter::reset() {
    running = false;
}

double PerformanceCounter::getElapsedMilliseconds() const {
    auto end = running ? std::chrono::high_resolution_clock::now() : endTime;
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime);
    return static_cast<double>(duration.count());
}

double PerformanceCounter::getElapsedMicroseconds() const {
    auto end = running ? std::chrono::high_resolution_clock::now() : endTime;
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - startTime);
    return static_cast<double>(duration.count());
}

// FrameTimer implementation
FrameTimer::FrameTimer()
    : deltaTime(0.0), fps(0.0), averageFPS(0.0), 
      minFPS(0.0), maxFPS(0.0), maxSamples(100) {
    lastFrameTime = std::chrono::high_resolution_clock::now();
    fpsHistory.reserve(maxSamples);
}

FrameTimer::~FrameTimer() {}

void FrameTimer::tick() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        currentTime - lastFrameTime);
    
    deltaTime = duration.count() / 1000000.0;
    fps = deltaTime > 0.0 ? 1.0 / deltaTime : 0.0;
    
    lastFrameTime = currentTime;
    
    fpsHistory.push_back(fps);
    if (fpsHistory.size() > maxSamples) {
        fpsHistory.erase(fpsHistory.begin());
    }
    
    updateFPSStats();
}

void FrameTimer::reset() {
    deltaTime = 0.0;
    fps = 0.0;
    averageFPS = 0.0;
    minFPS = 0.0;
    maxFPS = 0.0;
    fpsHistory.clear();
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

void FrameTimer::updateFPSStats() {
    if (fpsHistory.empty()) return;
    
    averageFPS = std::accumulate(fpsHistory.begin(), fpsHistory.end(), 0.0) / 
                 static_cast<double>(fpsHistory.size());
    
    minFPS = *std::min_element(fpsHistory.begin(), fpsHistory.end());
    maxFPS = *std::max_element(fpsHistory.begin(), fpsHistory.end());
}

// PerformanceProfiler implementation
PerformanceProfiler::PerformanceProfiler() : enabled(true) {}

PerformanceProfiler::~PerformanceProfiler() {}

PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

void PerformanceProfiler::beginSample(const std::string& name) {
    if (!enabled) return;
    
    activeSamples[name] = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endSample(const std::string& name) {
    if (!enabled) return;
    
    auto it = activeSamples.find(name);
    if (it == activeSamples.end()) return;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - it->second);
    
    double milliseconds = duration.count() / 1000.0;
    recordSample(name, milliseconds);
    
    activeSamples.erase(it);
}

PerformanceMetric* PerformanceProfiler::getMetric(const std::string& name) {
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        return &it->second;
    }
    return nullptr;
}

void PerformanceProfiler::update() {
    // Update logic
}

void PerformanceProfiler::reset() {
    for (auto& pair : metrics) {
        pair.second.value = 0.0;
        pair.second.minValue = 0.0;
        pair.second.maxValue = 0.0;
        pair.second.avgValue = 0.0;
        pair.second.sampleCount = 0;
    }
}

void PerformanceProfiler::clear() {
    metrics.clear();
    activeSamples.clear();
}

void PerformanceProfiler::recordSample(const std::string& name, double value) {
    auto& metric = metrics[name];
    metric.name = name;
    metric.value = value;
    
    if (metric.sampleCount == 0) {
        metric.minValue = value;
        metric.maxValue = value;
        metric.avgValue = value;
    } else {
        metric.minValue = std::min(metric.minValue, value);
        metric.maxValue = std::max(metric.maxValue, value);
        
        double totalSum = metric.avgValue * metric.sampleCount + value;
        metric.avgValue = totalSum / (metric.sampleCount + 1);
    }
    
    metric.sampleCount++;
}

// ScopedPerformanceTimer implementation
ScopedPerformanceTimer::ScopedPerformanceTimer(const std::string& name)
    : name(name) {
    startTime = std::chrono::high_resolution_clock::now();
    PerformanceProfiler::getInstance().beginSample(name);
}

ScopedPerformanceTimer::~ScopedPerformanceTimer() {
    PerformanceProfiler::getInstance().endSample(name);
}

// MemoryTracker implementation
MemoryTracker::MemoryTracker()
    : totalAllocated(0), currentUsage(0), peakUsage(0),
      allocationCount(0), deallocationCount(0) {}

MemoryTracker::~MemoryTracker() {}

MemoryTracker& MemoryTracker::getInstance() {
    static MemoryTracker instance;
    return instance;
}

void MemoryTracker::trackAllocation(size_t size) {
    totalAllocated += size;
    currentUsage += size;
    peakUsage = std::max(peakUsage, currentUsage);
    allocationCount++;
}

void MemoryTracker::trackDeallocation(size_t size) {
    currentUsage = currentUsage >= size ? currentUsage - size : 0;
    deallocationCount++;
}

void MemoryTracker::reset() {
    totalAllocated = 0;
    currentUsage = 0;
    peakUsage = 0;
    allocationCount = 0;
    deallocationCount = 0;
}

// PerformanceStats implementation
PerformanceStats::PerformanceStats()
    : drawCalls(0), triangles(0), vertices(0),
      frameTime(0.0), fps(0.0) {}

PerformanceStats::~PerformanceStats() {}

void PerformanceStats::update(double deltaTime) {
    frameTime = deltaTime;
    fps = deltaTime > 0.0 ? 1.0 / deltaTime : 0.0;
}

void PerformanceStats::reset() {
    drawCalls = 0;
    triangles = 0;
    vertices = 0;
}

// PerformanceGraph implementation
PerformanceGraph::PerformanceGraph(size_t maxSamples)
    : maxSamples(maxSamples), currentIndex(0) {
    samples.reserve(maxSamples);
}

PerformanceGraph::~PerformanceGraph() {}

void PerformanceGraph::addSample(double value) {
    if (samples.size() < maxSamples) {
        samples.push_back(value);
    } else {
        samples[currentIndex] = value;
        currentIndex = (currentIndex + 1) % maxSamples;
    }
}

void PerformanceGraph::clear() {
    samples.clear();
    currentIndex = 0;
}

double PerformanceGraph::getAverage() const {
    if (samples.empty()) return 0.0;
    
    return std::accumulate(samples.begin(), samples.end(), 0.0) / 
           static_cast<double>(samples.size());
}

double PerformanceGraph::getMin() const {
    if (samples.empty()) return 0.0;
    
    return *std::min_element(samples.begin(), samples.end());
}

double PerformanceGraph::getMax() const {
    if (samples.empty()) return 0.0;
    
    return *std::max_element(samples.begin(), samples.end());
}

void PerformanceGraph::render() {
    // Rendering would be done by the graphics system
    std::cout << "Performance Graph - Avg: " << getAverage() 
              << " Min: " << getMin() << " Max: " << getMax() << std::endl;
}

} // namespace Performance
} // namespace JJM
