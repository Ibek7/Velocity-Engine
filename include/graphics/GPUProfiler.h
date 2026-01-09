#ifndef GPU_PROFILER_H
#define GPU_PROFILER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief GPU timing query result
 */
struct GPUTimingResult {
    std::string name;
    double timeMs;              // GPU time in milliseconds
    uint64_t timestamp;         // Start timestamp
    int depth;                  // Nesting level for hierarchical profiling
    
    GPUTimingResult()
        : timeMs(0.0)
        , timestamp(0)
        , depth(0)
    {}
};

/**
 * @brief GPU profiler query
 */
class GPUQuery {
private:
    unsigned int m_queryStart;
    unsigned int m_queryEnd;
    std::string m_name;
    bool m_active;
    int m_depth;
    
public:
    GPUQuery(const std::string& name, int depth);
    ~GPUQuery();
    
    void begin();
    void end();
    bool isComplete() const;
    double getTimeMs() const;
    
    const std::string& getName() const { return m_name; }
    int getDepth() const { return m_depth; }
};

/**
 * @brief GPU performance profiler
 * 
 * Provides GPU timing queries for performance profiling.
 * Measures actual GPU execution time, not CPU submission time.
 */
class GPUProfiler {
private:
    struct Frame {
        std::vector<std::unique_ptr<GPUQuery>> queries;
        std::vector<GPUTimingResult> results;
        bool complete;
        
        Frame() : complete(false) {}
    };
    
    std::vector<Frame> m_frames;
    int m_currentFrame;
    int m_frameLatency;  // Number of frames to wait before reading results
    bool m_enabled;
    int m_currentDepth;
    
    // Statistics
    std::unordered_map<std::string, std::vector<double>> m_history;
    int m_historySize;
    
public:
    GPUProfiler();
    ~GPUProfiler();
    
    /**
     * @brief Initialize GPU profiler
     * @param frameLatency Number of frames to wait before reading results (usually 2-3)
     * @param historySize Number of frames to keep in history for averaging
     */
    void initialize(int frameLatency = 3, int historySize = 60);
    
    /**
     * @brief Shutdown profiler
     */
    void shutdown();
    
    /**
     * @brief Begin a new frame
     */
    void beginFrame();
    
    /**
     * @brief End current frame and collect results
     */
    void endFrame();
    
    /**
     * @brief Begin a GPU timing region
     * @param name Name of the region (e.g., "Shadow Pass", "Main Render")
     */
    void beginRegion(const std::string& name);
    
    /**
     * @brief End the current GPU timing region
     */
    void endRegion();
    
    /**
     * @brief Get timing results from previous frame
     * @return Vector of timing results
     */
    const std::vector<GPUTimingResult>& getResults() const;
    
    /**
     * @brief Get average time for a specific region over history
     * @param name Region name
     * @return Average time in milliseconds
     */
    double getAverageTime(const std::string& name) const;
    
    /**
     * @brief Get minimum time for a specific region over history
     */
    double getMinTime(const std::string& name) const;
    
    /**
     * @brief Get maximum time for a specific region over history
     */
    double getMaxTime(const std::string& name) const;
    
    /**
     * @brief Get total GPU frame time
     */
    double getTotalFrameTime() const;
    
    /**
     * @brief Enable/disable profiling
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief Clear history
     */
    void clearHistory();
    
    /**
     * @brief Get formatted profiling report
     */
    std::string generateReport() const;
    
    /**
     * @brief RAII helper for automatic begin/end
     */
    class ScopedRegion {
    private:
        GPUProfiler* m_profiler;
        
    public:
        ScopedRegion(GPUProfiler* profiler, const std::string& name)
            : m_profiler(profiler)
        {
            if (m_profiler && m_profiler->isEnabled()) {
                m_profiler->beginRegion(name);
            }
        }
        
        ~ScopedRegion() {
            if (m_profiler && m_profiler->isEnabled()) {
                m_profiler->endRegion();
            }
        }
        
        // Non-copyable
        ScopedRegion(const ScopedRegion&) = delete;
        ScopedRegion& operator=(const ScopedRegion&) = delete;
    };
    
private:
    Frame& getCurrentFrame();
    Frame& getReadFrame();
    void collectResults();
    void updateHistory(const std::vector<GPUTimingResult>& results);
};

// Convenience macro for scoped GPU profiling
#define GPU_PROFILE_SCOPE(profiler, name) \
    JJM::Graphics::GPUProfiler::ScopedRegion CONCAT(_gpu_profile_, __LINE__)(profiler, name)

} // namespace Graphics
} // namespace JJM

#endif // GPU_PROFILER_H
