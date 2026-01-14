#include "graphics/ShadowMapping.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// ShadowLight Implementation
ShadowLight::ShadowLight(Type type)
    : m_type(type)
    , m_castShadows(true)
    , m_shadowBias(0.005f)
    , m_normalOffsetBias(0.01f) {
    
    m_position[0] = m_position[1] = m_position[2] = 0.0f;
    m_direction[0] = 0.0f;
    m_direction[1] = -1.0f;
    m_direction[2] = 0.0f;
}

ShadowLight::~ShadowLight() {
}

void ShadowLight::setPosition(float x, float y, float z) {
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
}

void ShadowLight::setDirection(float x, float y, float z) {
    // Normalize direction
    float len = std::sqrt(x * x + y * y + z * z);
    if (len > 0.0f) {
        m_direction[0] = x / len;
        m_direction[1] = y / len;
        m_direction[2] = z / len;
    }
}

// ShadowMappingSystem Implementation
ShadowMappingSystem::ShadowMappingSystem()
    : m_quality(ShadowQuality::High)
    , m_filter(ShadowFilter::PCF)
    , m_nextLightId(1)
    , m_shadowShader(0)
    , m_debugVisualization(false)
    , m_visualizeCascades(false) {
}

ShadowMappingSystem::~ShadowMappingSystem() {
    m_lights.clear();
    m_cascades.clear();
}

void ShadowMappingSystem::setShadowQuality(ShadowQuality quality) {
    if (m_quality != quality) {
        m_quality = quality;
        initializeShadowMaps();
    }
}

void ShadowMappingSystem::setShadowFilter(ShadowFilter filter) {
    m_filter = filter;
}

void ShadowMappingSystem::setCSMConfig(const CSMConfig& config) {
    m_csmConfig = config;
    createCascadedShadowMaps();
}

uint32_t ShadowMappingSystem::registerLight(ShadowLight::Type type) {
    uint32_t id = m_nextLightId++;
    auto light = std::make_unique<ShadowLight>(type);
    m_lights.push_back(std::move(light));
    return id;
}

void ShadowMappingSystem::unregisterLight(uint32_t lightId) {
    // Remove light by ID (simplified)
    if (lightId > 0 && lightId <= m_lights.size()) {
        m_lights.erase(m_lights.begin() + (lightId - 1));
    }
}

ShadowLight* ShadowMappingSystem::getLight(uint32_t lightId) {
    if (lightId > 0 && lightId <= m_lights.size()) {
        return m_lights[lightId - 1].get();
    }
    return nullptr;
}

void ShadowMappingSystem::beginShadowPass() {
    m_stats.shadowMapsRendered = 0;
    m_stats.trianglesRendered = 0;
}

void ShadowMappingSystem::renderShadowMap(uint32_t lightId) {
    // TODO: Render shadow map for specific light
    m_stats.shadowMapsRendered++;
}

void ShadowMappingSystem::endShadowPass() {
    // Cleanup after shadow pass
}

void ShadowMappingSystem::updateCascades(const float* cameraViewMatrix,
                                        const float* cameraProjectionMatrix,
                                        float cameraNear, float cameraFar) {
    // Calculate cascade splits
    calculateCascadeSplits(cameraNear, cameraFar);
    
    // Calculate frustum corners for each cascade
    float prevSplit = cameraNear;
    
    for (uint32_t i = 0; i < m_csmConfig.cascadeCount; ++i) {
        ShadowCascade& cascade = m_cascades[i];
        float nearPlane = prevSplit;
        float farPlane = cascade.splitDistance;
        
        // Calculate frustum corners for this cascade
        float corners[8][3];
        ShadowHelper::calculateFrustumCorners(
            cameraViewMatrix, cameraProjectionMatrix,
            nearPlane, farPlane, corners
        );
        
        // Calculate light matrices
        calculateLightMatrices(i, cameraViewMatrix, corners);
        
        // Stabilize if enabled
        if (m_csmConfig.stabilizeCascades) {
            stabilizeCascade(cascade);
        }
        
        prevSplit = farPlane;
    }
}

const ShadowCascade* ShadowMappingSystem::getCascade(uint32_t index) const {
    if (index >= m_cascades.size()) {
        return nullptr;
    }
    return &m_cascades[index];
}

uint32_t ShadowMappingSystem::getShadowMapTexture(uint32_t lightId, uint32_t cascadeIndex) const {
    if (cascadeIndex >= m_cascades.size()) {
        return 0;
    }
    return m_cascades[cascadeIndex].depthTexture;
}

void ShadowMappingSystem::resetStatistics() {
    m_stats = Statistics();
}

void ShadowMappingSystem::initializeShadowMaps() {
    uint32_t resolution = getResolutionFromQuality(m_quality);
    
    // Initialize shadow maps based on quality
    // TODO: Create framebuffers and textures
}

void ShadowMappingSystem::createCascadedShadowMaps() {
    m_cascades.clear();
    m_cascades.resize(m_csmConfig.cascadeCount);
    
    uint32_t resolution = getResolutionFromQuality(m_quality);
    
    for (auto& cascade : m_cascades) {
        // TODO: Create framebuffer and depth texture for cascade
        cascade.framebuffer = 0;
        cascade.depthTexture = 0;
    }
}

void ShadowMappingSystem::calculateCascadeSplits(float cameraNear, float cameraFar) {
    float range = cameraFar - cameraNear;
    float ratio = cameraFar / cameraNear;
    
    for (uint32_t i = 0; i < m_csmConfig.cascadeCount; ++i) {
        float p = (i + 1) / static_cast<float>(m_csmConfig.cascadeCount);
        float logSplit = cameraNear * std::pow(ratio, p);
        float uniformSplit = cameraNear + range * p;
        
        // Blend between logarithmic and uniform splits using lambda
        float split = m_csmConfig.lambda * logSplit + (1.0f - m_csmConfig.lambda) * uniformSplit;
        
        // Clamp to max shadow distance
        split = std::min(split, m_csmConfig.maxShadowDistance);
        
        m_cascades[i].splitDistance = split;
    }
}

void ShadowMappingSystem::calculateLightMatrices(uint32_t cascadeIndex,
                                                 const float* cameraViewMatrix,
                                                 const float* frustumCorners) {
    if (m_lights.empty()) {
        return;
    }
    
    ShadowCascade& cascade = m_cascades[cascadeIndex];
    ShadowLight* light = m_lights[0].get(); // Use first light (typically sun)
    
    // Calculate the center of the frustum
    float center[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 8; ++i) {
        center[0] += frustumCorners[i * 3 + 0];
        center[1] += frustumCorners[i * 3 + 1];
        center[2] += frustumCorners[i * 3 + 2];
    }
    center[0] /= 8.0f;
    center[1] /= 8.0f;
    center[2] /= 8.0f;
    
    // Calculate light view matrix (looking at frustum center)
    const float* lightDir = light->getDirection();
    float lightPos[3] = {
        center[0] - lightDir[0] * 100.0f,
        center[1] - lightDir[1] * 100.0f,
        center[2] - lightDir[2] * 100.0f
    };
    
    // TODO: Properly calculate view and projection matrices
    // For now, initialize to identity
    for (int i = 0; i < 16; ++i) {
        cascade.viewMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        cascade.projectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
        cascade.viewProjectionMatrix[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
}

void ShadowMappingSystem::stabilizeCascade(ShadowCascade& cascade) {
    // Snap shadow map to texel grid to prevent shimmering
    uint32_t resolution = getResolutionFromQuality(m_quality);
    float texelSize = 2.0f / resolution;
    
    // Round position to nearest texel
    cascade.viewMatrix[12] = std::floor(cascade.viewMatrix[12] / texelSize) * texelSize;
    cascade.viewMatrix[13] = std::floor(cascade.viewMatrix[13] / texelSize) * texelSize;
}

uint32_t ShadowMappingSystem::getResolutionFromQuality(ShadowQuality quality) const {
    switch (quality) {
        case ShadowQuality::Low: return 512;
        case ShadowQuality::Medium: return 1024;
        case ShadowQuality::High: return 2048;
        case ShadowQuality::Ultra: return 4096;
        default: return 1024;
    }
}

// ShadowHelper Implementation
float ShadowHelper::calculateShadowBias(const float* normal,
                                       const float* lightDirection,
                                       float minBias, float maxBias) {
    // Calculate bias based on surface angle relative to light
    float ndotl = normal[0] * lightDirection[0] +
                  normal[1] * lightDirection[1] +
                  normal[2] * lightDirection[2];
    
    float bias = maxBias * (1.0f - std::abs(ndotl));
    return std::max(bias, minBias);
}

float ShadowHelper::sampleShadowPCF(uint32_t shadowMap,
                                   const float* shadowCoords,
                                   float bias,
                                   uint32_t kernelSize) {
    // TODO: Implement PCF shadow sampling
    return 0.0f;
}

void ShadowHelper::calculateFrustumCorners(const float* viewMatrix,
                                          const float* projectionMatrix,
                                          float nearPlane, float farPlane,
                                          float corners[8][3]) {
    // Calculate frustum corners in NDC
    float ndcCorners[8][4] = {
        {-1.0f, -1.0f, -1.0f, 1.0f}, // Near bottom-left
        { 1.0f, -1.0f, -1.0f, 1.0f}, // Near bottom-right
        { 1.0f,  1.0f, -1.0f, 1.0f}, // Near top-right
        {-1.0f,  1.0f, -1.0f, 1.0f}, // Near top-left
        {-1.0f, -1.0f,  1.0f, 1.0f}, // Far bottom-left
        { 1.0f, -1.0f,  1.0f, 1.0f}, // Far bottom-right
        { 1.0f,  1.0f,  1.0f, 1.0f}, // Far top-right
        {-1.0f,  1.0f,  1.0f, 1.0f}  // Far top-left
    };
    
    // TODO: Transform corners to world space using inverse view-projection
    // For now, just copy placeholder values
    for (int i = 0; i < 8; ++i) {
        corners[i][0] = ndcCorners[i][0];
        corners[i][1] = ndcCorners[i][1];
        corners[i][2] = ndcCorners[i][2];
    }
}

void ShadowHelper::transformFrustum(const float corners[8][3],
                                   const float* lightViewMatrix,
                                   float result[8][3]) {
    // Transform each corner by the light view matrix
    for (int i = 0; i < 8; ++i) {
        // TODO: Proper matrix-vector multiplication
        result[i][0] = corners[i][0];
        result[i][1] = corners[i][1];
        result[i][2] = corners[i][2];
    }
}

} // namespace Graphics
} // namespace JJM
