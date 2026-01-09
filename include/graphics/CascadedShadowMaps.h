#ifndef CASCADED_SHADOW_MAPS_H
#define CASCADED_SHADOW_MAPS_H

#include <vector>
#include <memory>

namespace JJM {
namespace Graphics {

/**
 * @brief Shadow cascade configuration
 */
struct ShadowCascade {
    float splitDistance;        // Far plane distance for this cascade
    float texelSize;           // Texel size in world space
    unsigned int depthTexture; // Shadow map texture
    unsigned int framebuffer;  // Framebuffer object
    float viewMatrix[16];      // Light view matrix
    float projMatrix[16];      // Light projection matrix
    float viewProjMatrix[16];  // Combined view-projection matrix
    
    ShadowCascade()
        : splitDistance(0.0f)
        , texelSize(0.0f)
        , depthTexture(0)
        , framebuffer(0)
    {}
};

/**
 * @brief PCF (Percentage Closer Filtering) quality settings
 */
enum class PCFQuality {
    Low,        // 2x2 samples
    Medium,     // 3x3 samples
    High,       // 5x5 samples
    Ultra       // 7x7 samples with Poisson disk
};

/**
 * @brief Cascaded shadow mapping system
 * 
 * Implements cascaded shadow maps (CSM) for high-quality directional
 * light shadows with PCF filtering and cascade blending.
 */
class CascadedShadowMaps {
private:
    std::vector<ShadowCascade> m_cascades;
    int m_numCascades;
    int m_shadowMapSize;
    PCFQuality m_pcfQuality;
    float m_cascadeSplitLambda;  // 0 = uniform, 1 = logarithmic
    float m_bias;
    float m_normalBias;
    bool m_stabilizeCascades;    // Prevent shadow shimmering
    
    // Shadow map parameters
    float m_lightDirection[3];
    float m_shadowDistance;      // Maximum shadow distance
    float m_fadeDistance;        // Distance to start fading out shadows
    
    // Filtering
    bool m_enablePCF;
    int m_pcfSamples;
    float m_pcfRadius;
    
public:
    CascadedShadowMaps();
    ~CascadedShadowMaps();
    
    /**
     * @brief Initialize shadow mapping system
     * @param numCascades Number of shadow cascades (typically 2-4)
     * @param shadowMapSize Size of each shadow map (e.g., 2048)
     */
    void initialize(int numCascades = 4, int shadowMapSize = 2048);
    
    /**
     * @brief Shutdown and free resources
     */
    void shutdown();
    
    /**
     * @brief Update cascade splits based on camera
     * @param cameraViewMatrix Camera view matrix
     * @param cameraProjMatrix Camera projection matrix
     * @param lightDir Directional light direction (normalized)
     */
    void update(const float* cameraViewMatrix, const float* cameraProjMatrix,
                const float* lightDir);
    
    /**
     * @brief Get cascade for rendering
     * @param index Cascade index
     */
    const ShadowCascade& getCascade(int index) const;
    
    /**
     * @brief Get number of cascades
     */
    int getNumCascades() const { return m_numCascades; }
    
    /**
     * @brief Set PCF quality
     */
    void setPCFQuality(PCFQuality quality);
    PCFQuality getPCFQuality() const { return m_pcfQuality; }
    
    /**
     * @brief Set shadow bias to reduce acne
     */
    void setBias(float bias, float normalBias) {
        m_bias = bias;
        m_normalBias = normalBias;
    }
    
    /**
     * @brief Set cascade split lambda (0 = uniform, 1 = logarithmic)
     */
    void setCascadeSplitLambda(float lambda) {
        m_cascadeSplitLambda = lambda;
    }
    
    /**
     * @brief Set maximum shadow distance
     */
    void setShadowDistance(float distance) { m_shadowDistance = distance; }
    float getShadowDistance() const { return m_shadowDistance; }
    
    /**
     * @brief Enable cascade stabilization to prevent shimmering
     */
    void setStabilizeCascades(bool enable) { m_stabilizeCascades = enable; }
    bool isStabilizingCascades() const { return m_stabilizeCascades; }
    
    /**
     * @brief Enable/disable PCF filtering
     */
    void setEnablePCF(bool enable) { m_enablePCF = enable; }
    bool isPCFEnabled() const { return m_enablePCF; }
    
    /**
     * @brief Set PCF filter radius
     */
    void setPCFRadius(float radius) { m_pcfRadius = radius; }
    
    /**
     * @brief Get shadow map size
     */
    int getShadowMapSize() const { return m_shadowMapSize; }
    
    /**
     * @brief Calculate cascade splits
     * @param nearPlane Camera near plane
     * @param farPlane Camera far plane
     * @return Vector of split distances
     */
    std::vector<float> calculateCascadeSplits(float nearPlane, float farPlane);
    
    /**
     * @brief Get cascade index for world position
     * @param worldPos World position
     * @param cameraViewMatrix Camera view matrix
     * @return Cascade index or -1 if outside shadow range
     */
    int getCascadeIndex(const float* worldPos, const float* cameraViewMatrix) const;
    
    /**
     * @brief Sample shadow map with PCF filtering
     * @param cascadeIndex Cascade to sample
     * @param worldPos World position
     * @return Shadow factor (0 = fully shadowed, 1 = fully lit)
     */
    float sampleShadowMapPCF(int cascadeIndex, const float* worldPos);
    
private:
    /**
     * @brief Create shadow map resources for a cascade
     */
    void createShadowMap(ShadowCascade& cascade);
    
    /**
     * @brief Calculate light view-projection matrix for cascade
     */
    void calculateLightMatrix(ShadowCascade& cascade, const float* frustumCorners,
                             const float* lightDir);
    
    /**
     * @brief Stabilize cascade to prevent shimmering
     */
    void stabilizeCascade(ShadowCascade& cascade);
    
    /**
     * @brief Extract frustum corners for cascade
     */
    void extractFrustumCorners(const float* viewMatrix, const float* projMatrix,
                              float nearPlane, float farPlane, float* corners);
};

/**
 * @brief Poisson disk samples for PCF
 */
class PoissonDisk {
public:
    static const float samples2x2[4][2];
    static const float samples3x3[9][2];
    static const float samples5x5[25][2];
    static const float samples7x7[49][2];
    
    static const float* getSamples(PCFQuality quality, int& outCount);
};

} // namespace Graphics
} // namespace JJM

#endif // CASCADED_SHADOW_MAPS_H
