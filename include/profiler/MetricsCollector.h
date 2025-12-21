#ifndef JJM_METRICS_COLLECTOR_H
#define JJM_METRICS_COLLECTOR_H

#include <string>
#include <map>
#include <vector>
#include <chrono>

namespace JJM {
namespace Profiler {

struct Metric {
    std::string name;
    double value;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> tags;
};

class MetricsCollector {
public:
    static MetricsCollector& getInstance();
    
    void recordMetric(const std::string& name, double value);
    void recordMetric(const std::string& name, double value, 
                     const std::map<std::string, std::string>& tags);
    
    void incrementCounter(const std::string& name, double amount = 1.0);
    void recordGauge(const std::string& name, double value);
    void recordTiming(const std::string& name, double milliseconds);
    
    std::vector<Metric> getMetrics(const std::string& name) const;
    std::vector<Metric> getAllMetrics() const;
    
    double getAverage(const std::string& name) const;
    double getMax(const std::string& name) const;
    double getMin(const std::string& name) const;
    
    void clear();
    void clearMetric(const std::string& name);

private:
    MetricsCollector();
    ~MetricsCollector();
    
    std::map<std::string, std::vector<Metric>> metrics;
};

class ScopedTimer {
public:
    ScopedTimer(const std::string& metricName);
    ~ScopedTimer();

private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
};

} // namespace Profiler
} // namespace JJM

#endif
