#include "graphics/CascadedShadowMaps.h"
#include <GL/glew.h>
#include <cmath>
#include <algorithm>
#include <cstring>

namespace JJM {
namespace Graphics {

// =============================================================================
// Poisson Disk Samples
// =============================================================================

const float PoissonDisk::samples2x2[4][2] = {
    {-0.5f, -0.5f}, {0.5f, -0.5f},
    {-0.5f, 0.5f}, {0.5f, 0.5f}
};

const float PoissonDisk::samples3x3[9][2] = {
    {-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f},
    {-1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f},
    {-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}
};

const float PoissonDisk::samples5x5[25][2] = {
    {-0.978698f, -0.904127f}, {-0.841121f, -0.394403f},
    {-0.725223f, -0.660474f}, {-0.683954f, 0.078459f},
    {-0.598821f, 0.568149f}, {-0.534030f, -0.911397f},
    {-0.485294f, 0.935218f}, {-0.440840f, -0.284834f},
    {-0.384230f, 0.327753f}, {-0.265012f, -0.636978f},
    {-0.229625f, 0.821165f}, {-0.053961f, -0.953809f},
    {0.007556f, 0.015664f}, {0.134131f, -0.345813f},
    {0.164041f, 0.539636f}, {0.282148f, -0.705026f},
    {0.365771f, 0.916750f}, {0.451464f, -0.019276f},
    {0.519456f, -0.440016f}, {0.598952f, 0.358093f},
    {0.650838f, -0.888629f}, {0.729826f, 0.668214f},
    {0.843479f, -0.294043f}, {0.900138f, 0.093760f},
    {0.966407f, -0.631450f}
};

const float PoissonDisk::samples7x7[49][2] = {
    // Precomputed Poisson disk with 49 samples
    {-0.934812f, -0.366741f}, {-0.914839f, 0.207662f},
    {-0.873324f, -0.765099f}, {-0.853734f, 0.615333f},
    {-0.793541f, -0.106815f}, {-0.778855f, 0.942316f},
    {-0.728814f, -0.478859f}, {-0.688256f, 0.346934f},
    {-0.647339f, -0.844230f}, {-0.619594f, 0.779990f},
    // ... more samples (truncated for brevity)
    {0.897763f, -0.324819f}, {0.934812f, 0.366741f}
};

const float* PoissonDisk::getSamples(PCFQuality quality, int& outCount) {
    switch (quality) {
        case PCFQuality::Low:
            outCount = 4;
            return &samples2x2[0][0];
        case PCFQuality::Medium:
            outCount = 9;
            return &samples3x3[0][0];
        case PCFQuality::High:
            outCount = 25;
            return &samples5x5[0][0];
        case PCFQuality::Ultra:
            outCount = 49;
            return &samples7x7[0][0];
        default:
            outCount = 9;
            return &samples3x3[0][0];
    }
}

// =============================================================================
// CascadedShadowMaps Implementation
// =============================================================================

CascadedShadowMaps::CascadedShadowMaps()
    : m_numCascades(4)
    , m_shadowMapSize(2048)
    , m_pcfQuality(PCFQuality::Medium)
    , m_cascadeSplitLambda(0.5f)
    , m_bias(0.005f)
    , m_normalBias(0.05f)
    , m_stabilizeCascades(true)
    , m_shadowDistance(100.0f)
    , m_fadeDistance(90.0f)
    , m_enablePCF(true)
    , m_pcfSamples(9)
    , m_pcfRadius(1.5f)
{
    m_lightDirection[0] = 0.0f;
    m_lightDirection[1] = -1.0f;
    m_lightDirection[2] = 0.0f;
}

CascadedShadowMaps::~CascadedShadowMaps() {
    shutdown();
}

void CascadedShadowMaps::initialize(int numCascades, int shadowMapSize) {
    m_numCascades = numCascades;
    m_shadowMapSize = shadowMapSize;
    
    m_cascades.resize(m_numCascades);
    
    // Create shadow maps for each cascade
    for (auto& cascade : m_cascades) {
        createShadowMap(cascade);
    }
}

void CascadedShadowMaps::shutdown() {
    for (auto& cascade : m_cascades) {
        if (cascade.depthTexture != 0) {
            glDeleteTextures(1, &cascade.depthTexture);
        }
        if (cascade.framebuffer != 0) {
            glDeleteFramebuffers(1, &cascade.framebuffer);
        }
    }
    m_cascades.clear();
}

void CascadedShadowMaps::update(const float* cameraViewMatrix, const float* cameraProjMatrix,
                                const float* lightDir) {
    std::memcpy(m_lightDirection, lightDir, sizeof(float) * 3);
    
    // Calculate cascade splits
    // TODO: Extract near/far from projection matrix
    float nearPlane = 0.1f;
    float farPlane = m_shadowDistance;
    
    auto splits = calculateCascadeSplits(nearPlane, farPlane);
    
    // Update each cascade
    for (int i = 0; i < m_numCascades; ++i) {
        float cascadeNear = (i == 0) ? nearPlane : splits[i - 1];
        float cascadeFar = splits[i];
        
        m_cascades[i].splitDistance = cascadeFar;
        
        // Extract frustum corners for this cascade
        float frustumCorners[8 * 3];
        extractFrustumCorners(cameraViewMatrix, cameraProjMatrix,
                            cascadeNear, cascadeFar, frustumCorners);
        
        // Calculate light matrices
        calculateLightMatrix(m_cascades[i], frustumCorners, lightDir);
        
        // Stabilize if enabled
        if (m_stabilizeCascades) {
            stabilizeCascade(m_cascades[i]);
        }
    }
}

const ShadowCascade& CascadedShadowMaps::getCascade(int index) const {
    static ShadowCascade empty;
    if (index >= 0 && index < m_numCascades) {
        return m_cascades[index];
    }
    return empty;
}

void CascadedShadowMaps::setPCFQuality(PCFQuality quality) {
    m_pcfQuality = quality;
    m_pcfSamples = 0;
    PoissonDisk::getSamples(quality, m_pcfSamples);
}

std::vector<float> CascadedShadowMaps::calculateCascadeSplits(float nearPlane, float farPlane) {
    std::vector<float> splits(m_numCascades);
    
    float lambda = m_cascadeSplitLambda;
    float range = farPlane - nearPlane;
    float ratio = farPlane / nearPlane;
    
    for (int i = 0; i < m_numCascades; ++i) {
        float p = static_cast<float>(i + 1) / m_numCascades;
        
        // Logarithmic split
        float logSplit = nearPlane * std::pow(ratio, p);
        
        // Uniform split
        float uniformSplit = nearPlane + range * p;
        
        // Blend between logarithmic and uniform
        splits[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
    }
    
    return splits;
}

int CascadedShadowMaps::getCascadeIndex(const float* worldPos, const float* cameraViewMatrix) const {
    // Transform world position to view space
    // TODO: Implement proper matrix-vector multiplication
    float viewZ = 0.0f; // Placeholder
    
    // Find appropriate cascade based on view-space depth
    for (int i = 0; i < m_numCascades; ++i) {
        if (viewZ < m_cascades[i].splitDistance) {
            return i;
        }
    }
    
    return -1; // Outside shadow range
}

float CascadedShadowMaps::sampleShadowMapPCF(int cascadeIndex, const float* worldPos) {
    if (cascadeIndex < 0 || cascadeIndex >= m_numCascades) {
        return 1.0f; // Fully lit
    }
    
    if (!m_enablePCF) {
        // Simple depth comparison
        // TODO: Implement basic shadow test
        return 1.0f;
    }
    
    // PCF filtering
    const ShadowCascade& cascade = m_cascades[cascadeIndex];
    
    // Transform world position to shadow map space
    // TODO: Apply cascade.viewProjMatrix to worldPos
    
    // Sample shadow map with Poisson disk
    int sampleCount;
    const float* samples = PoissonDisk::getSamples(m_pcfQuality, sampleCount);
    
    float shadow = 0.0f;
    float texelSize = cascade.texelSize * m_pcfRadius;
    
    for (int i = 0; i < sampleCount; ++i) {
        // TODO: Sample shadow map at offset position
        // shadow += sampleDepthMap(...)
    }
    
    shadow /= sampleCount;
    return shadow;
}

void CascadedShadowMaps::createShadowMap(ShadowCascade& cascade) {
    // Create depth texture
    glGenTextures(1, &cascade.depthTexture);
    glBindTexture(GL_TEXTURE_2D, cascade.depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowMapSize, m_shadowMapSize,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    
    // Create framebuffer
    glGenFramebuffers(1, &cascade.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, cascade.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                          cascade.depthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    // Check framebuffer completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        // Error handling
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CascadedShadowMaps::calculateLightMatrix(ShadowCascade& cascade, const float* frustumCorners,
                                              const float* lightDir) {
    // Calculate bounding sphere of frustum
    float center[3] = {0.0f, 0.0f, 0.0f};
    for (int i = 0; i < 8; ++i) {
        center[0] += frustumCorners[i * 3 + 0];
        center[1] += frustumCorners[i * 3 + 1];
        center[2] += frustumCorners[i * 3 + 2];
    }
    center[0] /= 8.0f;
    center[1] /= 8.0f;
    center[2] /= 8.0f;
    
    float radius = 0.0f;
    for (int i = 0; i < 8; ++i) {
        float dx = frustumCorners[i * 3 + 0] - center[0];
        float dy = frustumCorners[i * 3 + 1] - center[1];
        float dz = frustumCorners[i * 3 + 2] - center[2];
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        radius = std::max(radius, dist);
    }
    
    // Create orthographic projection
    // TODO: Implement proper light view and projection matrix creation
    
    cascade.texelSize = (2.0f * radius) / m_shadowMapSize;
}

void CascadedShadowMaps::stabilizeCascade(ShadowCascade& cascade) {
    // Snap shadow map to texel-sized increments to prevent shimmering
    // TODO: Implement shadow map stabilization
}

void CascadedShadowMaps::extractFrustumCorners(const float* viewMatrix, const float* projMatrix,
                                               float nearPlane, float farPlane, float* corners) {
    // Extract 8 corners of view frustum in world space
    // TODO: Implement frustum corner extraction
}

} // namespace Graphics
} // namespace JJM
