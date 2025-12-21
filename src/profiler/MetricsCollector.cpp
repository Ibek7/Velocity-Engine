#include "profiler/MetricsCollector.h"
#include <algorithm>
#include <limits>

namespace JJM {
namespace Profiler {

MetricsCollector& MetricsCollector::getInstance() {
    static MetricsCollector instance;
    return instance;
}

MetricsCollector::MetricsCollector() {
}

MetricsCollector::~MetricsCollector() {
}

void MetricsCollector::recordMetric(const std::string& name, double value) {
    recordMetric(name, value, {});
}

void MetricsCollector::recordMetric(const std::string& name, double value,
                                   const std::map<std::string, std::string>& tags) {
    Metric metric;
    metric.name = name;
    metric.value = value;
    metric.timestamp = std::chrono::system_clock::now();
    metric.tags = tags;
    
    metrics[name].push_back(metric);
}

void MetricsCollector::incrementCounter(const std::string& name, double amount) {
    double currentValue = 0.0;
    if (!metrics[name].empty()) {
        currentValue = metrics[name].back().value;
    }
    recordMetric(name, currentValue + amount);
}

void MetricsCollector::recordGauge(const std::string& name, double value) {
    recordMetric(name, value);
}

void MetricsCollector::recordTiming(const std::string& name, double milliseconds) {
    recordMetric(name, milliseconds);
}

std::vector<Metric> MetricsCollector::getMetrics(const std::string& name) const {
    auto it = metrics.find(name);
    if (it != metrics.end()) {
        return it->second;
    }
    return {};
}

std::vector<Metric> MetricsCollector::getAllMetrics() const {
    std::vector<Metric> result;
    for (const auto& pair : metrics) {
        result.insert(result.end(), pair.second.begin(), pair.second.end());
    }
    return result;
}

double MetricsCollector::getAverage(const std::string& name) const {
    auto it = metrics.find(name);
    if (it == metrics.end() || it->second.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (const auto& metric : it->second) {
        sum += metric.value;
    }
    return sum / it->second.size();
}

double MetricsCollector::getMax(const std::string& name) const {
    auto it = metrics.find(name);
    if (it == metrics.end() || it->second.empty()) {
        return 0.0;
    }
    
    double maxVal = std::numeric_limits<double>::lowest();
    for (const auto& metric : it->second) {
        maxVal = std::max(maxVal, metric.value);
    }
    return maxVal;
}

double MetricsCollector::getMin(const std::string& name) const {
    auto it = metrics.find(name);
    if (it == metrics.end() || it->second.empty()) {
        return 0.0;
    }
    
    double minVal = std::numeric_limits<double>::max();
    for (const auto& metric : it->second) {
        minVal = std::min(minVal, metric.value);
    }
    return minVal;
}

void MetricsCollector::clear() {
    metrics.clear();
}

void MetricsCollector::clearMetric(const std::string& name) {
    metrics.erase(name);
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& metricName)
    : name(metricName), startTime(std::chrono::high_resolution_clock::now()) {
}

ScopedTimer::~ScopedTimer() {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    MetricsCollector::getInstance().recordTiming(name, static_cast<double>(duration.count()));
}

} // namespace Profiler
} // namespace JJM
