#ifndef SHADOW_MAPPING_H
#define SHADOW_MAPPING_H

#include <cstdint>
#include <vector>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief Shadow map quality settings
 */
enum class ShadowQuality {
    Low,      // 512x512
    Medium,   // 1024x1024
    High,     // 2048x2048
    Ultra     // 4096x4096
};

/**
 * @brief Shadow filtering technique
 */
enum class ShadowFilter {
    None,     // Hard shadows
    PCF,      // Percentage Closer Filtering
    PCSS,     // Percentage Closer Soft Shadows
    VSM,      // Variance Shadow Maps
    ESM       // Exponential Shadow Maps
};

/**
 * @brief Cascaded shadow map configuration
 */
struct CSMConfig {
    uint32_t cascadeCount;              // Number of cascades (typically 3-4)
    float lambda;                        // Cascade split lambda (0-1)
    float maxShadowDistance;            // Maximum shadow rendering distance
    bool stabilizeCascades;             // Prevent flickering when camera rotates
    float cascadeBlendDistance;         // Distance to blend between cascades
    
    CSMConfig()
        : cascadeCount(4)
        , lambda(0.5f)
        , maxShadowDistance(500.0f)
        , stabilizeCascades(true)
        , cascadeBlendDistance(10.0f) {}
};

/**
 * @brief Single cascade data
 */
struct ShadowCascade {
    uint32_t framebuffer;
    uint32_t depthTexture;
    float splitDistance;               // Far plane for this cascade
    float viewMatrix[16];              // Light view matrix
    float projectionMatrix[16];        // Light projection matrix
    float viewProjectionMatrix[16];    // Combined VP matrix
    
    ShadowCascade()
        : framebuffer(0)
        , depthTexture(0)
        , splitDistance(0.0f) {
        // Initialize matrices to identity
        for (int i = 0; i < 16; ++i) {
            viewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            projectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
            viewProjectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        }
    }
};

/**
 * @brief Light for shadow casting
 */
class ShadowLight {
public:
    enum class Type {
        Directional,
        Point,
        Spot
    };
    
    ShadowLight(Type type);
    ~ShadowLight();
    
    Type getType() const { return m_type; }
    
    void setPosition(float x, float y, float z);
    void setDirection(float x, float y, float z);
    const float* getPosition() const { return m_position; }
    const float* getDirection() const { return m_direction; }
    
    void setCastShadows(bool cast) { m_castShadows = cast; }
    bool getCastShadows() const { return m_castShadows; }
    
    void setShadowBias(float bias) { m_shadowBias = bias; }
    float getShadowBias() const { return m_shadowBias; }
    
    void setNormalOffsetBias(float bias) { m_normalOffsetBias = bias; }
    float getNormalOffsetBias() const { return m_normalOffsetBias; }
    
private:
    Type m_type;
    float m_position[3];
    float m_direction[3];
    bool m_castShadows;
    float m_shadowBias;
    float m_normalOffsetBias;
};

/**
 * @brief Main shadow mapping system
 */
class ShadowMappingSystem {
public:
    ShadowMappingSystem();
    ~ShadowMappingSystem();
    
    // Configuration
    void setShadowQuality(ShadowQuality quality);
    void setShadowFilter(ShadowFilter filter);
    void setCSMConfig(const CSMConfig& config);
    
    ShadowQuality getShadowQuality() const { return m_quality; }
    ShadowFilter getShadowFilter() const { return m_filter; }
    const CSMConfig& getCSMConfig() const { return m_csmConfig; }
    
    // Light management
    uint32_t registerLight(ShadowLight::Type type);
    void unregisterLight(uint32_t lightId);
    ShadowLight* getLight(uint32_t lightId);
    
    // Shadow map generation
    void beginShadowPass();
    void renderShadowMap(uint32_t lightId);
    void endShadowPass();
    
    // Cascade management for directional lights
    void updateCascades(const float* cameraViewMatrix,
                       const float* cameraProjectionMatrix,
                       float cameraNear, float cameraFar);
    
    uint32_t getCascadeCount() const { return m_csmConfig.cascadeCount; }
    const ShadowCascade* getCascade(uint32_t index) const;
    
    // Shadow sampling
    uint32_t getShadowMapTexture(uint32_t lightId, uint32_t cascadeIndex = 0) const;
    
    // Statistics
    struct Statistics {
        uint32_t shadowMapsRendered;
        float shadowPassTime;
        uint32_t trianglesRendered;
        
        Statistics() 
            : shadowMapsRendered(0)
            , shadowPassTime(0.0f)
            , trianglesRendered(0) {}
    };
    
    const Statistics& getStatistics() const { return m_stats; }
    void resetStatistics();
    
    // Debug
    void enableDebugVisualization(bool enable) { m_debugVisualization = enable; }
    bool isDebugVisualizationEnabled() const { return m_debugVisualization; }
    void visualizeCascades(bool enable) { m_visualizeCascades = enable; }
    
private:
    void initializeShadowMaps();
    void createCascadedShadowMaps();
    void calculateCascadeSplits(float cameraNear, float cameraFar);
    void calculateLightMatrices(uint32_t cascadeIndex,
                               const float* cameraViewMatrix,
                               const float* frustumCorners);
    void stabilizeCascade(ShadowCascade& cascade);
    
    uint32_t getResolutionFromQuality(ShadowQuality quality) const;
    
    ShadowQuality m_quality;
    ShadowFilter m_filter;
    CSMConfig m_csmConfig;
    
    std::vector<ShadowCascade> m_cascades;
    std::vector<std::unique_ptr<ShadowLight>> m_lights;
    
    uint32_t m_nextLightId;
    uint32_t m_shadowShader;
    Statistics m_stats;
    
    bool m_debugVisualization;
    bool m_visualizeCascades;
};

/**
 * @brief Helper for shadow calculations
 */
class ShadowHelper {
public:
    // Calculate shadow bias based on normal and light direction
    static float calculateShadowBias(const float* normal, 
                                    const float* lightDirection,
                                    float minBias, float maxBias);
    
    // PCF shadow sampling
    static float sampleShadowPCF(uint32_t shadowMap,
                                const float* shadowCoords,
                                float bias,
                                uint32_t kernelSize);
    
    // Calculate frustum corners in world space
    static void calculateFrustumCorners(const float* viewMatrix,
                                       const float* projectionMatrix,
                                       float nearPlane, float farPlane,
                                       float corners[8][3]);
    
    // Transform frustum corners to light space
    static void transformFrustum(const float corners[8][3],
                                const float* lightViewMatrix,
                                float result[8][3]);
};

} // namespace Graphics
} // namespace JJM

#endif // SHADOW_MAPPING_H
