#pragma once

#include <string>
#include <vector>
#include <map>

// Dynamic resolution scaling based on performance
namespace Engine {

enum class ScalingMode {
    Fixed,          // No scaling
    Dynamic,        // Scale based on frame time
    Adaptive,       // Scale based on GPU load
    TargetFramerate // Maintain target FPS
};

enum class ScalingQuality {
    Performance,    // Prioritize FPS
    Balanced,       // Balance quality and performance
    Quality         // Prioritize visual quality
};

struct ResolutionLevel {
    float scaleFactor;      // 0.5 = 50%, 1.0 = 100%
    int width;
    int height;
    std::string label;      // "Low", "Medium", "High", etc.
};

class DynamicResolutionScaler {
public:
    DynamicResolutionScaler();
    ~DynamicResolutionScaler();

    // Initialization
    void initialize(int nativeWidth, int nativeHeight);
    void shutdown();

    // Configuration
    void setScalingMode(ScalingMode mode) { m_scalingMode = mode; }
    void setScalingQuality(ScalingQuality quality) { m_scalingQuality = quality; }
    void setTargetFrameTime(float ms) { m_targetFrameTime = ms; }
    void setTargetFPS(int fps) { m_targetFrameTime = 1000.0f / fps; }
    
    // Scale factor bounds
    void setMinScaleFactor(float factor) { m_minScaleFactor = factor; }
    void setMaxScaleFactor(float factor) { m_maxScaleFactor = factor; }
    void setScaleStep(float step) { m_scaleStep = step; }
    
    // Update
    void update(float deltaTime, float frameTime);
    void forceScaleFactor(float factor);
    
    // Query
    float getCurrentScaleFactor() const { return m_currentScaleFactor; }
    ResolutionLevel getCurrentResolution() const;
    int getCurrentWidth() const { return m_currentWidth; }
    int getCurrentHeight() const { return m_currentHeight; }
    int getNativeWidth() const { return m_nativeWidth; }
    int getNativeHeight() const { return m_nativeHeight; }
    
    // Performance tracking
    float getAverageFrameTime() const;
    float getAverageFPS() const;
    bool isScaling() const { return m_currentScaleFactor < 1.0f; }
    
    // Resolution levels
    void addResolutionLevel(const ResolutionLevel& level);
    const std::vector<ResolutionLevel>& getResolutionLevels() const { return m_resolutionLevels; }
    void setResolutionLevel(int index);
    
    // Upscaling
    void setUpscaleMethod(const std::string& method) { m_upscaleMethod = method; }
    const std::string& getUpscaleMethod() const { return m_upscaleMethod; }
    
    // Sharpening
    void enableSharpening(bool enable) { m_sharpeningEnabled = enable; }
    void setSharpeningAmount(float amount) { m_sharpeningAmount = amount; }
    
    // Stats
    struct Stats {
        float currentFPS;
        float averageFPS;
        float currentFrameTime;
        float averageFrameTime;
        float currentScaleFactor;
        int scaleUpCount;
        int scaleDownCount;
        float gpuTime;
        float cpuTime;
    };
    
    const Stats& getStats() const { return m_stats; }
    void resetStats();

private:
    void updateDynamic(float frameTime);
    void updateAdaptive(float frameTime);
    void updateTargetFramerate(float frameTime);
    
    void scaleUp();
    void scaleDown();
    void applyScaleFactor(float factor);
    
    void updateFrameTimeHistory(float frameTime);
    float calculateAverageFrameTime() const;

    // Configuration
    ScalingMode m_scalingMode;
    ScalingQuality m_scalingQuality;
    
    // Native resolution
    int m_nativeWidth;
    int m_nativeHeight;
    
    // Current resolution
    int m_currentWidth;
    int m_currentHeight;
    float m_currentScaleFactor;
    
    // Scale bounds
    float m_minScaleFactor;
    float m_maxScaleFactor;
    float m_scaleStep;
    
    // Target performance
    float m_targetFrameTime;
    
    // Frame time tracking
    std::vector<float> m_frameTimeHistory;
    int m_historySize;
    int m_historyIndex;
    
    // Upscaling
    std::string m_upscaleMethod;
    bool m_sharpeningEnabled;
    float m_sharpeningAmount;
    
    // Resolution presets
    std::vector<ResolutionLevel> m_resolutionLevels;
    
    // Stability
    float m_scaleTimer;
    float m_scaleDelay;
    
    // Stats
    Stats m_stats;
};

class DynamicResolutionSystem {
public:
    static DynamicResolutionSystem& getInstance();

    void initialize(int nativeWidth, int nativeHeight);
    void shutdown();
    void update(float deltaTime, float frameTime);

    DynamicResolutionScaler& getScaler() { return m_scaler; }
    const DynamicResolutionScaler& getScaler() const { return m_scaler; }
    
    // Convenience methods
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setTargetFPS(int fps) { m_scaler.setTargetFPS(fps); }
    float getCurrentScaleFactor() const { return m_scaler.getCurrentScaleFactor(); }

private:
    DynamicResolutionSystem();
    DynamicResolutionSystem(const DynamicResolutionSystem&) = delete;
    DynamicResolutionSystem& operator=(const DynamicResolutionSystem&) = delete;

    DynamicResolutionScaler m_scaler;
    bool m_enabled;
};

} // namespace Engine
