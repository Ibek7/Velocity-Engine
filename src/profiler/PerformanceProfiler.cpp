#include "../../include/profiler/PerformanceProfiler.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <thread>
#include <cmath>
#include <iostream>

namespace JJM {
namespace Utils {

// =============================================================================
// Performance Profiler Implementation
// =============================================================================

PerformanceProfiler* PerformanceProfiler::instance = nullptr;

PerformanceProfiler::PerformanceProfiler() {
    lastFrameTime = std::chrono::high_resolution_clock::now();
    frameTimeHistory.reserve(maxHistorySize);
}

PerformanceProfiler::~PerformanceProfiler() {
    if (activeSession) {
        endSession();
    }
}

PerformanceProfiler* PerformanceProfiler::getInstance() {
    if (!instance) {
        instance = new PerformanceProfiler();
    }
    return instance;
}

void PerformanceProfiler::cleanup() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

void PerformanceProfiler::beginFrame() {
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

void PerformanceProfiler::endFrame() {
    auto currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> deltaTime = currentTime - lastFrameTime;
    
    float frameTime = deltaTime.count();
    updateFrameStats(frameTime);
    
    lastFrameTime = currentTime;
    frameStats.frameCount++;
}

void PerformanceProfiler::updateFrameStats(float deltaTime) {
    std::lock_guard<std::mutex> lock(statsMutex);
    
    frameTimeHistory.push_back(deltaTime);
    if (frameTimeHistory.size() > maxHistorySize) {
        frameTimeHistory.erase(frameTimeHistory.begin());
    }
    
    frameStats.frameTime = deltaTime;
    frameStats.fps = (deltaTime > 0.0f) ? 1000.0f / deltaTime : 0.0f;
    
    if (!frameTimeHistory.empty()) {
        float sum = std::accumulate(frameTimeHistory.begin(), frameTimeHistory.end(), 0.0f);
        frameStats.avgFrameTime = sum / frameTimeHistory.size();
        
        frameStats.minFrameTime = *std::min_element(frameTimeHistory.begin(), frameTimeHistory.end());
        frameStats.maxFrameTime = *std::max_element(frameTimeHistory.begin(), frameTimeHistory.end());
        
        // Calculate variance
        float mean = frameStats.avgFrameTime;
        float variance = 0.0f;
        for (float time : frameTimeHistory) {
            float diff = time - mean;
            variance += diff * diff;
        }
        frameStats.variance = variance / frameTimeHistory.size();
        
        // Calculate percentiles
        std::vector<float> sorted = frameTimeHistory;
        std::sort(sorted.begin(), sorted.end());
        
        size_t idx95 = static_cast<size_t>(sorted.size() * 0.95);
        size_t idx99 = static_cast<size_t>(sorted.size() * 0.99);
        
        frameStats.percentile95 = sorted[std::min(idx95, sorted.size() - 1)];
        frameStats.percentile99 = sorted[std::min(idx99, sorted.size() - 1)];
    }
}

void PerformanceProfiler::trackAllocation(size_t size, const std::string& category) {
    if (!memoryTrackingEnabled) return;
    
    std::lock_guard<std::mutex> lock(statsMutex);
    
    memoryStats.totalAllocated += size;
    memoryStats.currentUsage += size;
    memoryStats.allocationCount++;
    
    if (memoryStats.currentUsage > memoryStats.peakUsage) {
        memoryStats.peakUsage = memoryStats.currentUsage;
    }
    
    memoryStats.categoryUsage[category] += size;
}

void PerformanceProfiler::trackDeallocation(size_t size, const std::string& category) {
    if (!memoryTrackingEnabled) return;
    
    std::lock_guard<std::mutex> lock(statsMutex);
    
    memoryStats.totalFreed += size;
    memoryStats.currentUsage = (memoryStats.currentUsage >= size) ? 
                                memoryStats.currentUsage - size : 0;
    memoryStats.deallocationCount++;
    
    if (memoryStats.categoryUsage.find(category) != memoryStats.categoryUsage.end()) {
        size_t& catUsage = memoryStats.categoryUsage[category];
        catUsage = (catUsage >= size) ? catUsage - size : 0;
    }
}

void PerformanceProfiler::resetMemoryStats() {
    std::lock_guard<std::mutex> lock(statsMutex);
    memoryStats = MemoryStats();
}

void PerformanceProfiler::trackDrawCall(int triangles) {
    gpuStats.drawCallCount++;
    gpuStats.triangleCount += triangles;
}

void PerformanceProfiler::beginProfile(const std::string& name) {
    if (!profilingEnabled) return;
    
    std::lock_guard<std::mutex> lock(profileMutex);
    
    ProfileEntry entry;
    entry.name = name;
    entry.startTime = std::chrono::high_resolution_clock::now();
    entry.depth = currentDepth++;
    entry.threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
    
    profileEntries.push_back(entry);
}

void PerformanceProfiler::endProfile(const std::string& name) {
    if (!profilingEnabled) return;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    
    std::lock_guard<std::mutex> lock(profileMutex);
    
    currentDepth--;
    
    // Find matching start entry
    for (auto it = profileEntries.rbegin(); it != profileEntries.rend(); ++it) {
        if (it->name == name && !it->completed) {
            it->endTime = endTime;
            it->completed = true;
            
            std::chrono::duration<double, std::micro> duration = endTime - it->startTime;
            it->duration = duration.count();
            
            // Add to history
            timerHistory[name].push_back(it->duration);
            if (timerHistory[name].size() > maxHistorySize) {
                timerHistory[name].erase(timerHistory[name].begin());
            }
            
            // Move to completed entries
            completedEntries.push_back(*it);
            
            // Write to session if active
            if (activeSession) {
                writeSessionData(*it);
            }
            
            break;
        }
    }
}

double PerformanceProfiler::getAverageTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = timerHistory.find(name);
    if (it != timerHistory.end() && !it->second.empty()) {
        double sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
        return sum / it->second.size();
    }
    return 0.0;
}

double PerformanceProfiler::getMinTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = timerHistory.find(name);
    if (it != timerHistory.end() && !it->second.empty()) {
        return *std::min_element(it->second.begin(), it->second.end());
    }
    return 0.0;
}

double PerformanceProfiler::getMaxTime(const std::string& name) const {
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = timerHistory.find(name);
    if (it != timerHistory.end() && !it->second.empty()) {
        return *std::max_element(it->second.begin(), it->second.end());
    }
    return 0.0;
}

const std::vector<double>& PerformanceProfiler::getTimeHistory(const std::string& name) const {
    static std::vector<double> empty;
    
    std::lock_guard<std::mutex> lock(profileMutex);
    
    auto it = timerHistory.find(name);
    if (it != timerHistory.end()) {
        return it->second;
    }
    return empty;
}

void PerformanceProfiler::beginSession(const std::string& name, const std::string& filepath) {
    if (activeSession) {
        endSession();
    }
    
    std::string path = filepath.empty() ? (name + ".json") : filepath;
    activeSession = std::make_unique<ProfilerSession>(name, path);
}

void PerformanceProfiler::endSession() {
    activeSession.reset();
}

void PerformanceProfiler::writeSessionData(const ProfileEntry& entry) {
    if (activeSession && activeSession->isOpen()) {
        activeSession->writeProfile(entry);
    }
}

std::string PerformanceProfiler::getCurrentThreadName() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

std::string PerformanceProfiler::generateReport() const {
    std::ostringstream report;
    
    report << "=== Performance Report ===\n\n";
    
    // Frame stats
    report << "Frame Statistics:\n";
    report << "  FPS: " << std::fixed << std::setprecision(2) << frameStats.fps << "\n";
    report << "  Frame Time: " << frameStats.frameTime << " ms\n";
    report << "  Avg Frame Time: " << frameStats.avgFrameTime << " ms\n";
    report << "  Min/Max: " << frameStats.minFrameTime << " / " << frameStats.maxFrameTime << " ms\n";
    report << "  95th/99th Percentile: " << frameStats.percentile95 << " / " 
           << frameStats.percentile99 << " ms\n";
    report << "  Total Frames: " << frameStats.frameCount << "\n\n";
    
    // Memory stats
    report << "Memory Statistics:\n";
    report << "  Current Usage: " << (memoryStats.currentUsage / 1024.0 / 1024.0) << " MB\n";
    report << "  Peak Usage: " << (memoryStats.peakUsage / 1024.0 / 1024.0) << " MB\n";
    report << "  Total Allocated: " << (memoryStats.totalAllocated / 1024.0 / 1024.0) << " MB\n";
    report << "  Total Freed: " << (memoryStats.totalFreed / 1024.0 / 1024.0) << " MB\n";
    report << "  Allocation Count: " << memoryStats.allocationCount << "\n";
    report << "  Deallocation Count: " << memoryStats.deallocationCount << "\n\n";
    
    // GPU stats
    report << "GPU Statistics:\n";
    report << "  Draw Calls: " << gpuStats.drawCallCount << "\n";
    report << "  Triangles: " << gpuStats.triangleCount << "\n";
    report << "  Texture Memory: " << gpuStats.textureMemory << " MB\n";
    report << "  Buffer Memory: " << gpuStats.bufferMemory << " MB\n\n";
    
    // Profiling data
    report << "Profiling Data:\n";
    for (const auto& pair : timerHistory) {
        report << "  " << pair.first << ":\n";
        report << "    Avg: " << (getAverageTime(pair.first) / 1000.0) << " ms\n";
        report << "    Min: " << (getMinTime(pair.first) / 1000.0) << " ms\n";
        report << "    Max: " << (getMaxTime(pair.first) / 1000.0) << " ms\n";
    }
    
    return report.str();
}

void PerformanceProfiler::printReport() const {
    std::cout << generateReport();
}

void PerformanceProfiler::exportToCSV(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "Name,AvgTime(ms),MinTime(ms),MaxTime(ms),CallCount\n";
    
    for (const auto& pair : timerHistory) {
        file << pair.first << ","
             << (getAverageTime(pair.first) / 1000.0) << ","
             << (getMinTime(pair.first) / 1000.0) << ","
             << (getMaxTime(pair.first) / 1000.0) << ","
             << pair.second.size() << "\n";
    }
    
    file.close();
}

void PerformanceProfiler::exportToJSON(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"frameStats\": {\n";
    file << "    \"fps\": " << frameStats.fps << ",\n";
    file << "    \"frameTime\": " << frameStats.frameTime << ",\n";
    file << "    \"avgFrameTime\": " << frameStats.avgFrameTime << ",\n";
    file << "    \"minFrameTime\": " << frameStats.minFrameTime << ",\n";
    file << "    \"maxFrameTime\": " << frameStats.maxFrameTime << "\n";
    file << "  },\n";
    file << "  \"memoryStats\": {\n";
    file << "    \"currentUsage\": " << memoryStats.currentUsage << ",\n";
    file << "    \"peakUsage\": " << memoryStats.peakUsage << "\n";
    file << "  },\n";
    file << "  \"profileData\": [\n";
    
    bool first = true;
    for (const auto& pair : timerHistory) {
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"name\": \"" << pair.first << "\",\n";
        file << "      \"avgTime\": " << getAverageTime(pair.first) << ",\n";
        file << "      \"minTime\": " << getMinTime(pair.first) << ",\n";
        file << "      \"maxTime\": " << getMaxTime(pair.first) << "\n";
        file << "    }";
    }
    
    file << "\n  ]\n";
    file << "}\n";
    
    file.close();
}

std::vector<std::string> PerformanceProfiler::detectBottlenecks(float thresholdMs) const {
    std::vector<std::string> bottlenecks;
    
    for (const auto& pair : timerHistory) {
        double avgTimeMs = getAverageTime(pair.first) / 1000.0;
        if (avgTimeMs > thresholdMs) {
            bottlenecks.push_back(pair.first + " (" + std::to_string(avgTimeMs) + " ms)");
        }
    }
    
    return bottlenecks;
}

float PerformanceProfiler::calculateMemoryFragmentation() const {
    size_t allocatedNotFreed = memoryStats.totalAllocated - memoryStats.totalFreed;
    if (allocatedNotFreed == 0) return 0.0f;
    
    return (float)(memoryStats.peakUsage - memoryStats.currentUsage) / allocatedNotFreed;
}

void PerformanceProfiler::clear() {
    std::lock_guard<std::mutex> lock1(statsMutex);
    std::lock_guard<std::mutex> lock2(profileMutex);
    
    frameTimeHistory.clear();
    profileEntries.clear();
    completedEntries.clear();
    timerHistory.clear();
    
    frameStats = FrameStats();
    gpuStats = GPUStats();
}

// =============================================================================
// Scoped Timer Implementation
// =============================================================================

ScopedTimer::ScopedTimer(const std::string& timerName)
    : name(timerName), active(true) {
    PerformanceProfiler::getInstance()->beginProfile(name);
}

ScopedTimer::~ScopedTimer() {
    if (active) {
        PerformanceProfiler::getInstance()->endProfile(name);
    }
}

// =============================================================================
// Profiler Session Implementation
// =============================================================================

ProfilerSession::ProfilerSession(const std::string& sessionName, const std::string& file)
    : name(sessionName), filepath(file) {
    
    outputStream.open(filepath);
    sessionStart = std::chrono::high_resolution_clock::now();
    
    if (outputStream.is_open()) {
        writeHeader();
    }
}

ProfilerSession::~ProfilerSession() {
    if (outputStream.is_open()) {
        writeFooter();
        outputStream.close();
    }
}

void ProfilerSession::writeHeader() {
    outputStream << "{\"otherData\": {},\"traceEvents\":[\n";
    outputStream.flush();
}

void ProfilerSession::writeFooter() {
    outputStream << "\n]}";
    outputStream.flush();
}

void ProfilerSession::writeProfile(const ProfileEntry& entry) {
    if (!outputStream.is_open()) return;
    
    if (profileCount++ > 0) {
        outputStream << ",\n";
    }
    
    std::chrono::duration<double, std::micro> start = entry.startTime - sessionStart;
    
    outputStream << "{"
                 << "\"cat\":\"function\","
                 << "\"dur\":" << entry.duration << ","
                 << "\"name\":\"" << entry.name << "\","
                 << "\"ph\":\"X\","
                 << "\"pid\":0,"
                 << "\"tid\":" << entry.threadId << ","
                 << "\"ts\":" << start.count()
                 << "}";
    
    outputStream.flush();
}

// =============================================================================
// Budget Monitor Implementation
// =============================================================================

void BudgetMonitor::checkFrameTimeBudget(float frameTime) {
    budgetViolations.clear();
    
    if (frameTime > budget.frameTimeBudget) {
        budgetViolations.push_back("Frame time budget exceeded: " + 
                                   std::to_string(frameTime) + " ms > " + 
                                   std::to_string(budget.frameTimeBudget) + " ms");
    }
}

void BudgetMonitor::checkMemoryBudget(size_t usage) {
    if (usage > budget.memoryBudget) {
        budgetViolations.push_back("Memory budget exceeded: " + 
                                   std::to_string(usage / 1024.0 / 1024.0) + " MB > " + 
                                   std::to_string(budget.memoryBudget / 1024.0 / 1024.0) + " MB");
    }
}

void BudgetMonitor::checkDrawCallBudget(int drawCalls) {
    if (drawCalls > budget.drawCallBudget) {
        budgetViolations.push_back("Draw call budget exceeded: " + 
                                   std::to_string(drawCalls) + " > " + 
                                   std::to_string(budget.drawCallBudget));
    }
}

// =============================================================================
// Frame Analyzer Implementation
// =============================================================================

void FrameAnalyzer::addFrame(float frameTime) {
    frameTimes.push_back(frameTime);
    if (frameTimes.size() > maxSamples) {
        frameTimes.erase(frameTimes.begin());
    }
}

float FrameAnalyzer::getMean() const {
    if (frameTimes.empty()) return 0.0f;
    float sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0f);
    return sum / frameTimes.size();
}

float FrameAnalyzer::getMedian() const {
    if (frameTimes.empty()) return 0.0f;
    
    std::vector<float> sorted = frameTimes;
    std::sort(sorted.begin(), sorted.end());
    
    size_t mid = sorted.size() / 2;
    if (sorted.size() % 2 == 0) {
        return (sorted[mid - 1] + sorted[mid]) / 2.0f;
    } else {
        return sorted[mid];
    }
}

float FrameAnalyzer::getStandardDeviation() const {
    if (frameTimes.empty()) return 0.0f;
    
    float mean = getMean();
    float variance = 0.0f;
    
    for (float time : frameTimes) {
        float diff = time - mean;
        variance += diff * diff;
    }
    
    variance /= frameTimes.size();
    return std::sqrt(variance);
}

float FrameAnalyzer::getPercentile(float p) const {
    if (frameTimes.empty()) return 0.0f;
    
    std::vector<float> sorted = frameTimes;
    std::sort(sorted.begin(), sorted.end());
    
    size_t idx = static_cast<size_t>(sorted.size() * p);
    return sorted[std::min(idx, sorted.size() - 1)];
}

std::vector<int> FrameAnalyzer::detectSpikes(float threshold) const {
    std::vector<int> spikes;
    float mean = getMean();
    float stdDev = getStandardDeviation();
    
    for (size_t i = 0; i < frameTimes.size(); i++) {
        if (std::abs(frameTimes[i] - mean) > threshold * stdDev) {
            spikes.push_back(static_cast<int>(i));
        }
    }
    
    return spikes;
}

int FrameAnalyzer::getSpikeCount(float threshold) const {
    return static_cast<int>(detectSpikes(threshold).size());
}

bool FrameAnalyzer::isFramePacingStable(float tolerance) const {
    if (frameTimes.size() < 2) return true;
    
    float mean = getMean();
    int stableCount = 0;
    
    for (float time : frameTimes) {
        if (std::abs(time - mean) / mean <= tolerance) {
            stableCount++;
        }
    }
    
    return (float)stableCount / frameTimes.size() >= 0.9f;  // 90% stable
}

float FrameAnalyzer::getFramePacingVariability() const {
    return getStandardDeviation() / getMean();
}

std::string FrameAnalyzer::generateHistogram(int bins) const {
    if (frameTimes.empty()) return "No data";
    
    float minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
    float maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
    float binSize = (maxTime - minTime) / bins;
    
    std::vector<int> histogram(bins, 0);
    
    for (float time : frameTimes) {
        int bin = static_cast<int>((time - minTime) / binSize);
        bin = std::min(bin, bins - 1);
        histogram[bin]++;
    }
    
    std::ostringstream oss;
    oss << "Frame Time Histogram:\n";
    
    for (int i = 0; i < bins; i++) {
        float rangeStart = minTime + i * binSize;
        float rangeEnd = rangeStart + binSize;
        
        oss << std::fixed << std::setprecision(2)
            << rangeStart << "-" << rangeEnd << " ms: ";
        
        int barLength = (histogram[i] * 50) / frameTimes.size();
        oss << std::string(barLength, '=') << " (" << histogram[i] << ")\n";
    }
    
    return oss.str();
}

// =============================================================================
// Call Stack Profiler Implementation
// =============================================================================

void CallStackProfiler::push(const std::string& name) {
    CallStackEntry entry;
    entry.functionName = name;
    
    if (stack.empty()) {
        root.children.push_back(entry);
        stack.push_back(&root.children.back());
    } else {
        stack.back()->children.push_back(entry);
        stack.push_back(&stack.back()->children.back());
    }
}

void CallStackProfiler::pop() {
    if (!stack.empty()) {
        stack.pop_back();
    }
}

void CallStackProfiler::reset() {
    root = CallStackEntry();
    stack.clear();
}

std::vector<CallStackEntry> CallStackProfiler::getFlattenedProfile() const {
    std::vector<CallStackEntry> flattened;
    
    std::function<void(const CallStackEntry&)> flatten = [&](const CallStackEntry& entry) {
        flattened.push_back(entry);
        for (const auto& child : entry.children) {
            flatten(child);
        }
    };
    
    for (const auto& child : root.children) {
        flatten(child);
    }
    
    return flattened;
}

std::string CallStackProfiler::generateFlameGraph() const {
    std::ostringstream oss;
    
    std::function<void(const CallStackEntry&, int)> printTree = 
        [&](const CallStackEntry& entry, int depth) {
        if (!entry.functionName.empty()) {
            oss << std::string(depth * 2, ' ') << entry.functionName 
                << " (" << entry.inclusiveTime << " us)\n";
        }
        
        for (const auto& child : entry.children) {
            printTree(child, depth + 1);
        }
    };
    
    printTree(root, 0);
    return oss.str();
}

} // namespace Utils
} // namespace JJM
