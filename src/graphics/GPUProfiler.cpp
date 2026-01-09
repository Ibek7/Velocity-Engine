#include "graphics/GPUProfiler.h"
#include <GL/glew.h>
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace JJM {
namespace Graphics {

// =============================================================================
// GPUQuery Implementation
// =============================================================================

GPUQuery::GPUQuery(const std::string& name, int depth)
    : m_name(name)
    , m_active(false)
    , m_depth(depth)
{
    glGenQueries(1, &m_queryStart);
    glGenQueries(1, &m_queryEnd);
}

GPUQuery::~GPUQuery() {
    glDeleteQueries(1, &m_queryStart);
    glDeleteQueries(1, &m_queryEnd);
}

void GPUQuery::begin() {
    glQueryCounter(m_queryStart, GL_TIMESTAMP);
    m_active = true;
}

void GPUQuery::end() {
    glQueryCounter(m_queryEnd, GL_TIMESTAMP);
    m_active = false;
}

bool GPUQuery::isComplete() const {
    if (m_active) return false;
    
    GLint available = 0;
    glGetQueryObjectiv(m_queryEnd, GL_QUERY_RESULT_AVAILABLE, &available);
    return available != 0;
}

double GPUQuery::getTimeMs() const {
    GLuint64 startTime, endTime;
    glGetQueryObjectui64v(m_queryStart, GL_QUERY_RESULT, &startTime);
    glGetQueryObjectui64v(m_queryEnd, GL_QUERY_RESULT, &endTime);
    
    // Convert nanoseconds to milliseconds
    return (endTime - startTime) / 1000000.0;
}

// =============================================================================
// GPUProfiler Implementation
// =============================================================================

GPUProfiler::GPUProfiler()
    : m_currentFrame(0)
    , m_frameLatency(3)
    , m_enabled(true)
    , m_currentDepth(0)
    , m_historySize(60)
{}

GPUProfiler::~GPUProfiler() {
    shutdown();
}

void GPUProfiler::initialize(int frameLatency, int historySize) {
    m_frameLatency = frameLatency;
    m_historySize = historySize;
    m_frames.resize(frameLatency + 1);
    m_currentFrame = 0;
    m_enabled = true;
}

void GPUProfiler::shutdown() {
    m_frames.clear();
    m_history.clear();
    m_enabled = false;
}

void GPUProfiler::beginFrame() {
    if (!m_enabled) return;
    
    Frame& frame = getCurrentFrame();
    frame.queries.clear();
    frame.results.clear();
    frame.complete = false;
    m_currentDepth = 0;
}

void GPUProfiler::endFrame() {
    if (!m_enabled) return;
    
    // Collect results from old frame
    collectResults();
    
    // Move to next frame
    m_currentFrame = (m_currentFrame + 1) % m_frames.size();
}

void GPUProfiler::beginRegion(const std::string& name) {
    if (!m_enabled) return;
    
    Frame& frame = getCurrentFrame();
    auto query = std::make_unique<GPUQuery>(name, m_currentDepth);
    query->begin();
    frame.queries.push_back(std::move(query));
    
    m_currentDepth++;
}

void GPUProfiler::endRegion() {
    if (!m_enabled) return;
    
    m_currentDepth--;
    
    Frame& frame = getCurrentFrame();
    if (!frame.queries.empty()) {
        frame.queries.back()->end();
    }
}

const std::vector<GPUTimingResult>& GPUProfiler::getResults() const {
    static std::vector<GPUTimingResult> emptyResults;
    
    if (!m_enabled || m_frames.empty()) {
        return emptyResults;
    }
    
    const Frame& readFrame = const_cast<GPUProfiler*>(this)->getReadFrame();
    return readFrame.results;
}

double GPUProfiler::getAverageTime(const std::string& name) const {
    auto it = m_history.find(name);
    if (it == m_history.end() || it->second.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
    return sum / it->second.size();
}

double GPUProfiler::getMinTime(const std::string& name) const {
    auto it = m_history.find(name);
    if (it == m_history.end() || it->second.empty()) {
        return 0.0;
    }
    
    return *std::min_element(it->second.begin(), it->second.end());
}

double GPUProfiler::getMaxTime(const std::string& name) const {
    auto it = m_history.find(name);
    if (it == m_history.end() || it->second.empty()) {
        return 0.0;
    }
    
    return *std::max_element(it->second.begin(), it->second.end());
}

double GPUProfiler::getTotalFrameTime() const {
    const auto& results = getResults();
    double total = 0.0;
    
    // Sum only top-level regions (depth 0)
    for (const auto& result : results) {
        if (result.depth == 0) {
            total += result.timeMs;
        }
    }
    
    return total;
}

void GPUProfiler::clearHistory() {
    m_history.clear();
}

std::string GPUProfiler::generateReport() const {
    std::ostringstream oss;
    
    oss << "=== GPU Profiling Report ===\n";
    oss << std::fixed << std::setprecision(3);
    
    const auto& results = getResults();
    if (results.empty()) {
        oss << "No profiling data available.\n";
        return oss.str();
    }
    
    // Sort by depth for hierarchical display
    std::vector<GPUTimingResult> sorted = results;
    std::sort(sorted.begin(), sorted.end(),
        [](const GPUTimingResult& a, const GPUTimingResult& b) {
            if (a.depth != b.depth) return a.depth < b.depth;
            return a.timestamp < b.timestamp;
        });
    
    for (const auto& result : sorted) {
        // Indent based on depth
        for (int i = 0; i < result.depth; ++i) {
            oss << "  ";
        }
        
        oss << result.name << ": " << result.timeMs << " ms";
        
        // Add average if available
        double avg = getAverageTime(result.name);
        if (avg > 0.0) {
            oss << " (avg: " << avg << " ms, "
                << "min: " << getMinTime(result.name) << " ms, "
                << "max: " << getMaxTime(result.name) << " ms)";
        }
        
        oss << "\n";
    }
    
    oss << "\nTotal Frame Time: " << getTotalFrameTime() << " ms\n";
    oss << "=============================\n";
    
    return oss.str();
}

GPUProfiler::Frame& GPUProfiler::getCurrentFrame() {
    return m_frames[m_currentFrame];
}

GPUProfiler::Frame& GPUProfiler::getReadFrame() {
    int readFrame = (m_currentFrame + 1) % m_frames.size();
    return m_frames[readFrame];
}

void GPUProfiler::collectResults() {
    Frame& readFrame = getReadFrame();
    
    if (readFrame.complete) {
        return;  // Already collected
    }
    
    // Check if all queries are complete
    bool allComplete = true;
    for (const auto& query : readFrame.queries) {
        if (!query->isComplete()) {
            allComplete = false;
            break;
        }
    }
    
    if (!allComplete) {
        return;  // Wait for next frame
    }
    
    // Collect all timing results
    readFrame.results.clear();
    for (const auto& query : readFrame.queries) {
        GPUTimingResult result;
        result.name = query->getName();
        result.timeMs = query->getTimeMs();
        result.depth = query->getDepth();
        readFrame.results.push_back(result);
    }
    
    // Update history
    updateHistory(readFrame.results);
    readFrame.complete = true;
}

void GPUProfiler::updateHistory(const std::vector<GPUTimingResult>& results) {
    for (const auto& result : results) {
        auto& history = m_history[result.name];
        history.push_back(result.timeMs);
        
        // Keep only recent history
        if (history.size() > static_cast<size_t>(m_historySize)) {
            history.erase(history.begin());
        }
    }
}

} // namespace Graphics
} // namespace JJM
