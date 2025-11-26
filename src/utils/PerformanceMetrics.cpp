#include "utils/PerformanceMetrics.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace JJM {
namespace Utils {

// MetricData implementation
void PerformanceMonitor::MetricData::addSample(double value) {
    samples.push_back(value);
    total += value;
    
    if (samples.size() == 1) {
        minValue = maxValue = value;
    } else {
        minValue = std::min(minValue, value);
        maxValue = std::max(maxValue, value);
    }
    
    if (static_cast<int>(samples.size()) > maxSamples) {
        double oldest = samples.front();
        samples.pop_front();
        total -= oldest;
    }
}

PerformanceMetric PerformanceMonitor::MetricData::getMetric(const std::string& name) const {
    PerformanceMetric metric;
    metric.name = name;
    metric.sampleCount = static_cast<int>(samples.size());
    
    if (metric.sampleCount > 0) {
        metric.averageTime = total / metric.sampleCount;
        metric.minTime = minValue;
        metric.maxTime = maxValue;
    }
    
    return metric;
}

// PerformanceMonitor implementation
PerformanceMonitor* PerformanceMonitor::instance = nullptr;

PerformanceMonitor::PerformanceMonitor()
    : frameTime(0), updateTime(0), renderTime(0),
      drawCalls(0), memoryUsage(0) {
}

PerformanceMonitor* PerformanceMonitor::getInstance() {
    if (!instance) {
        instance = new PerformanceMonitor();
    }
    return instance;
}

PerformanceMonitor::~PerformanceMonitor() {
}

void PerformanceMonitor::startTimer(const std::string& name) {
    activeTimers[name] = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endTimer(const std::string& name) {
    auto it = activeTimers.find(name);
    if (it != activeTimers.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - it->second).count() / 1000.0;
        
        if (metrics.find(name) == metrics.end()) {
            metrics[name] = MetricData();
        }
        metrics[name].addSample(duration);
        
        activeTimers.erase(it);
    }
}

void PerformanceMonitor::recordFrameTime(double time) {
    frameTime = time;
    if (metrics.find("FrameTime") == metrics.end()) {
        metrics["FrameTime"] = MetricData();
    }
    metrics["FrameTime"].addSample(time);
}

void PerformanceMonitor::recordUpdateTime(double time) {
    updateTime = time;
    if (metrics.find("UpdateTime") == metrics.end()) {
        metrics["UpdateTime"] = MetricData();
    }
    metrics["UpdateTime"].addSample(time);
}

void PerformanceMonitor::recordRenderTime(double time) {
    renderTime = time;
    if (metrics.find("RenderTime") == metrics.end()) {
        metrics["RenderTime"] = MetricData();
    }
    metrics["RenderTime"].addSample(time);
}

void PerformanceMonitor::incrementDrawCalls(int count) {
    drawCalls += count;
}

void PerformanceMonitor::setMemoryUsage(size_t bytes) {
    memoryUsage = bytes;
}

PerformanceMetric PerformanceMonitor::getMetric(const std::string& name) const {
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        return it->second.getMetric(name);
    }
    return PerformanceMetric();
}

std::vector<std::string> PerformanceMonitor::getMetricNames() const {
    std::vector<std::string> names;
    for (const auto& pair : metrics) {
        names.push_back(pair.first);
    }
    return names;
}

void PerformanceMonitor::resetDrawCalls() {
    drawCalls = 0;
}

void PerformanceMonitor::reset() {
    frameTime = 0;
    updateTime = 0;
    renderTime = 0;
    drawCalls = 0;
}

void PerformanceMonitor::clear() {
    metrics.clear();
    activeTimers.clear();
    reset();
}

void PerformanceMonitor::printReport() const {
    std::cout << generateReport() << std::endl;
}

std::string PerformanceMonitor::generateReport() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    
    oss << "=== Performance Report ===" << std::endl;
    oss << "FPS: " << getFPS() << std::endl;
    oss << "Frame Time: " << frameTime << " ms" << std::endl;
    oss << "Update Time: " << updateTime << " ms" << std::endl;
    oss << "Render Time: " << renderTime << " ms" << std::endl;
    oss << "Draw Calls: " << drawCalls << std::endl;
    oss << "Memory Usage: " << (memoryUsage / 1024.0 / 1024.0) << " MB" << std::endl;
    
    oss << "\n=== Custom Metrics ===" << std::endl;
    for (const auto& pair : metrics) {
        auto metric = pair.second.getMetric(pair.first);
        if (metric.sampleCount > 0) {
            oss << pair.first << ": "
                << "avg=" << metric.averageTime << "ms "
                << "min=" << metric.minTime << "ms "
                << "max=" << metric.maxTime << "ms "
                << "samples=" << metric.sampleCount << std::endl;
        }
    }
    
    return oss.str();
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& timerName)
    : name(timerName), startTime(std::chrono::high_resolution_clock::now()) {
}

ScopedTimer::~ScopedTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        endTime - startTime).count() / 1000.0;
    
    auto* monitor = PerformanceMonitor::getInstance();
    if (monitor->metrics.find(name) == monitor->metrics.end()) {
        monitor->metrics[name] = MetricData();
    }
    monitor->metrics[name].addSample(duration);
}

// FrameAnalyzer implementation
FrameAnalyzer::FrameAnalyzer(int maxFrames) : maxFrames(maxFrames) {
}

void FrameAnalyzer::recordFrame(double frameTime, double updateTime,
                               double renderTime, int drawCalls) {
    FrameData data;
    data.frameTime = frameTime;
    data.updateTime = updateTime;
    data.renderTime = renderTime;
    data.drawCalls = drawCalls;
    
    frames.push_back(data);
    
    if (static_cast<int>(frames.size()) > maxFrames) {
        frames.pop_front();
    }
}

double FrameAnalyzer::getAverageFrameTime() const {
    if (frames.empty()) return 0;
    
    double sum = 0;
    for (const auto& frame : frames) {
        sum += frame.frameTime;
    }
    return sum / frames.size();
}

double FrameAnalyzer::getAverageUpdateTime() const {
    if (frames.empty()) return 0;
    
    double sum = 0;
    for (const auto& frame : frames) {
        sum += frame.updateTime;
    }
    return sum / frames.size();
}

double FrameAnalyzer::getAverageRenderTime() const {
    if (frames.empty()) return 0;
    
    double sum = 0;
    for (const auto& frame : frames) {
        sum += frame.renderTime;
    }
    return sum / frames.size();
}

double FrameAnalyzer::getAverageFPS() const {
    double avgFrameTime = getAverageFrameTime();
    return avgFrameTime > 0 ? 1000.0 / avgFrameTime : 0;
}

double FrameAnalyzer::getMinFrameTime() const {
    if (frames.empty()) return 0;
    
    double minTime = frames[0].frameTime;
    for (const auto& frame : frames) {
        minTime = std::min(minTime, frame.frameTime);
    }
    return minTime;
}

double FrameAnalyzer::getMaxFrameTime() const {
    if (frames.empty()) return 0;
    
    double maxTime = frames[0].frameTime;
    for (const auto& frame : frames) {
        maxTime = std::max(maxTime, frame.frameTime);
    }
    return maxTime;
}

void FrameAnalyzer::clear() {
    frames.clear();
}

} // namespace Utils
} // namespace JJM
