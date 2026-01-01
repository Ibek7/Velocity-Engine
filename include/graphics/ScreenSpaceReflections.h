#pragma once

#include <string>

namespace Engine {

/**
 * @brief Quality levels for SSR
 */
enum class SSRQuality {
    Low,
    Medium,
    High,
    Ultra
};

/**
 * @brief Ray march settings for SSR
 */
struct RayMarchSettings {
    int maxSteps;           // Maximum ray march steps
    float stepSize;         // Step size for ray marching
    float maxDistance;      // Maximum ray march distance
    float thickness;        // Surface thickness for intersection
    int binarySearchSteps;  // Binary search refinement steps
    
    RayMarchSettings()
        : maxSteps(64)
        , stepSize(1.0f)
        , maxDistance(100.0f)
        , thickness(0.5f)
        , binarySearchSteps(8)
    {}
};

/**
 * @brief Temporal filtering settings
 */
struct TemporalSettings {
    bool enabled;           // Enable temporal filtering
    float blendFactor;      // Blend factor with history
    int historyFrames;      // Number of history frames
    bool jitterEnabled;     // Enable ray jittering
    
    TemporalSettings()
        : enabled(true)
        , blendFactor(0.1f)
        , historyFrames(2)
        , jitterEnabled(true)
    {}
};

/**
 * @brief Reflection fade settings
 */
struct FadeSettings {
    float edgeFadeStart;    // Edge fade start distance
    float edgeFadeEnd;      // Edge fade end distance
    float distanceFadeStart; // Distance fade start
    float distanceFadeEnd;   // Distance fade end
    bool fadeAtEdges;       // Fade reflections at screen edges
    bool fadeWithDistance;  // Fade with distance
    
    FadeSettings()
        : edgeFadeStart(0.7f)
        , edgeFadeEnd(0.9f)
        , distanceFadeStart(50.0f)
        , distanceFadeEnd(100.0f)
        , fadeAtEdges(true)
        , fadeWithDistance(true)
    {}
};

/**
 * @brief Screen space reflections configuration
 */
struct SSRConfig {
    SSRQuality quality;
    RayMarchSettings rayMarch;
    TemporalSettings temporal;
    FadeSettings fade;
    bool enabled;
    float intensity;        // Reflection intensity
    float roughnessThreshold; // Max roughness for reflections
    bool useHiZ;           // Use hierarchical depth buffer
    int hiZLevels;         // Number of HiZ mipmap levels
    
    SSRConfig()
        : quality(SSRQuality::High)
        , enabled(true)
        , intensity(1.0f)
        , roughnessThreshold(0.8f)
        , useHiZ(true)
        , hiZLevels(5)
    {}
};

/**
 * @brief Screen space reflection data
 */
struct SSRData {
    unsigned int reflectionTexture;  // Reflection color
    unsigned int confidenceTexture;  // Reflection confidence
    unsigned int hitMaskTexture;     // Ray hit mask
    unsigned int historyTexture;     // Previous frame
    float coverage;                  // Reflection coverage
    
    SSRData()
        : reflectionTexture(0)
        , confidenceTexture(0)
        , hitMaskTexture(0)
        , historyTexture(0)
        , coverage(0.0f)
    {}
};

/**
 * @brief Screen space reflections system
 * 
 * Implements real-time screen space reflections with:
 * - Ray marching in screen space
 * - Temporal filtering for stability
 * - Roughness-based reflections
 * - Hierarchical depth buffer optimization
 * - Edge and distance fading
 */
class ScreenSpaceReflections {
public:
    ScreenSpaceReflections();
    ~ScreenSpaceReflections();
    
    // Configuration
    void setConfig(const SSRConfig& config);
    SSRConfig& getConfig() { return m_config; }
    const SSRConfig& getConfig() const { return m_config; }
    
    // Initialization
    void initialize(int width, int height);
    void shutdown();
    void resize(int width, int height);
    
    // Rendering
    void render(unsigned int colorTexture, 
                unsigned int normalTexture,
                unsigned int depthTexture,
                unsigned int roughnessTexture,
                const float* viewMatrix,
                const float* projMatrix,
                const float* invViewMatrix,
                const float* invProjMatrix);
    
    // G-buffer input (for deferred rendering)
    void renderDeferred(unsigned int colorTexture,
                       unsigned int gbuffer0,  // Normal + Roughness
                       unsigned int gbuffer1,  // Position + Metallic
                       unsigned int depthTexture,
                       const float* viewMatrix,
                       const float* projMatrix);
    
    // Result access
    SSRData getReflectionData() const { return m_data; }
    unsigned int getReflectionTexture() const { return m_data.reflectionTexture; }
    unsigned int getConfidenceTexture() const { return m_data.confidenceTexture; }
    
    // HiZ generation
    void generateHiZ(unsigned int depthTexture);
    unsigned int getHiZTexture() const { return m_hiZTexture; }
    
    // Statistics
    float getCoverage() const { return m_data.coverage; }
    float getAverageRayLength() const { return m_avgRayLength; }
    int getRayCount() const { return m_rayCount; }
    
private:
    void createFramebuffers();
    void destroyFramebuffers();
    void createShaders();
    
    // Ray marching
    void rayMarch(unsigned int normalTexture,
                 unsigned int depthTexture,
                 unsigned int roughnessTexture,
                 const float* viewMatrix,
                 const float* projMatrix);
    
    // Temporal filtering
    void temporalFilter();
    
    // Resolve and composite
    void resolve(unsigned int colorTexture);
    
    // Utilities
    void calculateJitter(float& x, float& y) const;
    float calculateFade(float x, float y, float depth, float roughness) const;
    void updateStatistics();
    
    SSRConfig m_config;
    SSRData m_data;
    
    // Framebuffers
    unsigned int m_rayMarchFBO;
    unsigned int m_resolveFBO;
    unsigned int m_temporalFBO;
    
    // Textures
    unsigned int m_hiZTexture;
    unsigned int m_velocityTexture;
    
    // Shaders
    unsigned int m_rayMarchShader;
    unsigned int m_resolveShader;
    unsigned int m_temporalShader;
    unsigned int m_hiZShader;
    
    // Resolution
    int m_width;
    int m_height;
    
    // Statistics
    float m_avgRayLength;
    int m_rayCount;
    int m_frameCount;
};

/**
 * @brief Global SSR system
 */
class SSRSystem {
public:
    static SSRSystem& getInstance();
    
    void initialize(int width, int height);
    void shutdown();
    
    ScreenSpaceReflections& getSSR() { return m_ssr; }
    
private:
    SSRSystem() = default;
    ScreenSpaceReflections m_ssr;
};

} // namespace Engine
