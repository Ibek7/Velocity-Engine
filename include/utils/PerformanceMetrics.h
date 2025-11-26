#ifndef PERFORMANCE_METRICS_H
#define PERFORMANCE_METRICS_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <deque>

namespace JJM {
namespace Utils {

struct PerformanceMetric {
    std::string name;
    double averageTime;
    double minTime;
    double maxTime;
    int sampleCount;
    
    PerformanceMetric()
        : averageTime(0), minTime(0), maxTime(0), sampleCount(0) {}
};

class PerformanceMonitor {
private:
    struct MetricData {
        std::deque<double> samples;
        double total;
        double minValue;
        double maxValue;
        int maxSamples;
        
        MetricData(int maxSamples = 100)
            : total(0), minValue(0), maxValue(0), maxSamples(maxSamples) {}
        
        void addSample(double value);
        PerformanceMetric getMetric(const std::string& name) const;
    };
    
    std::map<std::string, MetricData> metrics;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> activeTimers;
    
    double frameTime;
    double updateTime;
    double renderTime;
    int drawCalls;
    size_t memoryUsage;
    
    static PerformanceMonitor* instance;
    PerformanceMonitor();
    
public:
    static PerformanceMonitor* getInstance();
    ~PerformanceMonitor();
    
    void startTimer(const std::string& name);
    void endTimer(const std::string& name);
    
    void recordFrameTime(double time);
    void recordUpdateTime(double time);
    void recordRenderTime(double time);
    void incrementDrawCalls(int count = 1);
    void setMemoryUsage(size_t bytes);
    
    PerformanceMetric getMetric(const std::string& name) const;
    std::vector<std::string> getMetricNames() const;
    
    double getFrameTime() const { return frameTime; }
    double getUpdateTime() const { return updateTime; }
    double getRenderTime() const { return renderTime; }
    int getDrawCalls() const { return drawCalls; }
    size_t getMemoryUsage() const { return memoryUsage; }
    
    double getFPS() const { return frameTime > 0 ? 1000.0 / frameTime : 0; }
    
    void resetDrawCalls();
    void reset();
    void clear();
    
    void printReport() const;
    std::string generateReport() const;
};

class ScopedTimer {
private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    
public:
    ScopedTimer(const std::string& timerName);
    ~ScopedTimer();
};

class FrameAnalyzer {
private:
    struct FrameData {
        double frameTime;
        double updateTime;
        double renderTime;
        int drawCalls;
        
        FrameData()
            : frameTime(0), updateTime(0), renderTime(0), drawCalls(0) {}
    };
    
    std::deque<FrameData> frames;
    int maxFrames;
    
public:
    FrameAnalyzer(int maxFrames = 300);
    
    void recordFrame(double frameTime, double updateTime, double renderTime, int drawCalls);
    
    double getAverageFrameTime() const;
    double getAverageUpdateTime() const;
    double getAverageRenderTime() const;
    double getAverageFPS() const;
    
    double getMinFrameTime() const;
    double getMaxFrameTime() const;
    
    int getFrameCount() const { return static_cast<int>(frames.size()); }
    
    void clear();
};

} // namespace Utils
} // namespace JJM

#endif // PERFORMANCE_METRICS_H
