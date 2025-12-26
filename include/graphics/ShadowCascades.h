#pragma once

#include <vector>
#include <string>

namespace Engine {

/**
 * @brief Cascaded shadow map configuration
 */
struct ShadowCascade {
    float splitDistance;        // Split plane distance from camera
    float viewMatrix[16];       // Light view matrix for this cascade
    float projMatrix[16];       // Light projection matrix for this cascade
    unsigned int framebuffer;   // Shadow map framebuffer
    unsigned int depthTexture;  // Shadow depth texture
    int resolution;             // Shadow map resolution
    float splitNear;            // Near plane for this split
    float splitFar;             // Far plane for this split
};

/**
 * @brief Shadow quality presets
 */
enum class ShadowQuality {
    Low,        // 1 cascade, 512x512
    Medium,     // 2 cascades, 1024x1024
    High,       // 3 cascades, 2048x2048
    Ultra       // 4 cascades, 4096x4096
};

/**
 * @brief PCF filter size for soft shadows
 */
enum class PCFFilterSize {
    None,       // No filtering (hard shadows)
    Size3x3,    // 3x3 kernel
    Size5x5,    // 5x5 kernel
    Size7x7     // 7x7 kernel
};

/**
 * @brief Shadow settings
 */
struct ShadowSettings {
    int numCascades;            // Number of shadow cascades
    int resolution;             // Shadow map resolution per cascade
    float lambda;               // Split scheme lambda (0=uniform, 1=logarithmic)
    float bias;                 // Shadow bias to prevent acne
    float normalBias;           // Normal-based bias
    float cascadeBlendDistance; // Distance to blend between cascades
    PCFFilterSize filterSize;   // PCF filter size
    bool stabilizeProjection;   // Stabilize shadow maps (prevent shimmering)
    
    ShadowSettings()
        : numCascades(3)
        , resolution(2048)
        , lambda(0.5f)
        , bias(0.0005f)
        , normalBias(0.001f)
        , cascadeBlendDistance(10.0f)
        , filterSize(PCFFilterSize::Size5x5)
        , stabilizeProjection(true)
    {}
};

/**
 * @brief Statistics for shadow rendering
 */
struct ShadowStats {
    int numCascades;
    int totalResolution;        // Total pixels across all cascades
    float lastUpdateTime;       // Time taken to update shadows (ms)
    int drawCalls;              // Number of draw calls for shadow maps
    
    ShadowStats()
        : numCascades(0)
        , totalResolution(0)
        , lastUpdateTime(0.0f)
        , drawCalls(0)
    {}
};

/**
 * @brief Cascaded shadow mapping system
 */
class ShadowCascadeSystem {
public:
    ShadowCascadeSystem();
    ~ShadowCascadeSystem();
    
    // Initialization
    void initialize();
    void shutdown();
    
    // Configuration
    void setSettings(const ShadowSettings& settings);
    const ShadowSettings& getSettings() const { return m_settings; }
    void setQuality(ShadowQuality quality);
    
    // Light setup
    void setLightDirection(float x, float y, float z);
    void setLightViewMatrix(const float matrix[16]);
    
    // Camera setup
    void setCameraViewProj(const float view[16], const float proj[16]);
    void setCameraPosition(float x, float y, float z);
    void setCameraFrustum(float fov, float aspect, float nearPlane, float farPlane);
    
    // Shadow map updates
    void update(float deltaTime);
    void calculateCascadeSplits();
    void updateCascadeMatrices();
    
    // Rendering
    void beginShadowPass(int cascadeIndex);
    void endShadowPass();
    unsigned int getShadowMapTexture(int cascadeIndex) const;
    
    // Shadow map access
    int getNumCascades() const { return static_cast<int>(m_cascades.size()); }
    const ShadowCascade& getCascade(int index) const { return m_cascades[index]; }
    const std::vector<float>& getSplitDistances() const { return m_splitDistances; }
    
    // Bias management
    void setDepthBias(float bias) { m_settings.bias = bias; }
    void setNormalBias(float bias) { m_settings.normalBias = bias; }
    float calculateAdaptiveBias(float distance) const;
    
    // Cascade selection
    int selectCascade(float depth) const;
    float getCascadeBlendFactor(float depth, int cascadeIndex) const;
    
    // Debug
    void enableDebugVisualization(bool enable) { m_debugVisualization = enable; }
    const ShadowStats& getStats() const { return m_stats; }
    void resetStats();
    
private:
    void createShadowMaps();
    void destroyShadowMaps();
    void calculateSplitDistances();
    void calculateCascadeMatrix(int index);
    void stabilizeShadowMap(int index);
    
    float calculateLogSplit(float near, float far, float i, float n) const;
    float calculateUniformSplit(float near, float far, float i, float n) const;
    
    ShadowSettings m_settings;
    std::vector<ShadowCascade> m_cascades;
    std::vector<float> m_splitDistances;
    
    // Light data
    float m_lightDirX, m_lightDirY, m_lightDirZ;
    float m_lightView[16];
    
    // Camera data
    float m_cameraView[16];
    float m_cameraProj[16];
    float m_cameraX, m_cameraY, m_cameraZ;
    float m_cameraFov;
    float m_cameraAspect;
    float m_cameraNear;
    float m_cameraFar;
    
    // Debug
    bool m_debugVisualization;
    ShadowStats m_stats;
};

/**
 * @brief Global shadow cascade system singleton
 */
class ShadowSystem {
public:
    static ShadowSystem& getInstance();
    
    void initialize();
    void shutdown();
    
    ShadowCascadeSystem& getCascadeSystem() { return m_cascadeSystem; }
    
    // Quality presets
    void setQuality(ShadowQuality quality);
    
    // Enable/disable shadows
    void enable(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    ShadowSystem() : m_enabled(true) {}
    ShadowSystem(const ShadowSystem&) = delete;
    ShadowSystem& operator=(const ShadowSystem&) = delete;
    
    ShadowCascadeSystem m_cascadeSystem;
    bool m_enabled;
};

} // namespace Engine
