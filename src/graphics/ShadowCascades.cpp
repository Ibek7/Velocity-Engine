#include "graphics/ShadowCascades.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Engine {

ShadowCascadeSystem::ShadowCascadeSystem()
    : m_lightDirX(0.0f)
    , m_lightDirY(-1.0f)
    , m_lightDirZ(0.0f)
    , m_cameraX(0.0f)
    , m_cameraY(0.0f)
    , m_cameraZ(0.0f)
    , m_cameraFov(60.0f)
    , m_cameraAspect(16.0f / 9.0f)
    , m_cameraNear(0.1f)
    , m_cameraFar(1000.0f)
    , m_debugVisualization(false)
{
    std::memset(m_lightView, 0, sizeof(m_lightView));
    std::memset(m_cameraView, 0, sizeof(m_cameraView));
    std::memset(m_cameraProj, 0, sizeof(m_cameraProj));
    
    // Identity matrices
    for (int i = 0; i < 4; ++i) {
        m_lightView[i * 4 + i] = 1.0f;
        m_cameraView[i * 4 + i] = 1.0f;
        m_cameraProj[i * 4 + i] = 1.0f;
    }
}

ShadowCascadeSystem::~ShadowCascadeSystem() {
    shutdown();
}

void ShadowCascadeSystem::initialize() {
    createShadowMaps();
    calculateCascadeSplits();
}

void ShadowCascadeSystem::shutdown() {
    destroyShadowMaps();
}

void ShadowCascadeSystem::setSettings(const ShadowSettings& settings) {
    bool needRecreate = (settings.numCascades != m_settings.numCascades ||
                         settings.resolution != m_settings.resolution);
    
    m_settings = settings;
    
    if (needRecreate) {
        destroyShadowMaps();
        createShadowMaps();
    }
    
    calculateCascadeSplits();
}

void ShadowCascadeSystem::setQuality(ShadowQuality quality) {
    ShadowSettings settings = m_settings;
    
    switch (quality) {
        case ShadowQuality::Low:
            settings.numCascades = 1;
            settings.resolution = 512;
            settings.filterSize = PCFFilterSize::None;
            break;
        case ShadowQuality::Medium:
            settings.numCascades = 2;
            settings.resolution = 1024;
            settings.filterSize = PCFFilterSize::Size3x3;
            break;
        case ShadowQuality::High:
            settings.numCascades = 3;
            settings.resolution = 2048;
            settings.filterSize = PCFFilterSize::Size5x5;
            break;
        case ShadowQuality::Ultra:
            settings.numCascades = 4;
            settings.resolution = 4096;
            settings.filterSize = PCFFilterSize::Size7x7;
            break;
    }
    
    setSettings(settings);
}

void ShadowCascadeSystem::setLightDirection(float x, float y, float z) {
    m_lightDirX = x;
    m_lightDirY = y;
    m_lightDirZ = z;
    
    // Normalize
    float length = std::sqrt(x * x + y * y + z * z);
    if (length > 0.0f) {
        m_lightDirX /= length;
        m_lightDirY /= length;
        m_lightDirZ /= length;
    }
}

void ShadowCascadeSystem::setLightViewMatrix(const float matrix[16]) {
    std::memcpy(m_lightView, matrix, sizeof(m_lightView));
}

void ShadowCascadeSystem::setCameraViewProj(const float view[16], const float proj[16]) {
    std::memcpy(m_cameraView, view, sizeof(m_cameraView));
    std::memcpy(m_cameraProj, proj, sizeof(m_cameraProj));
}

void ShadowCascadeSystem::setCameraPosition(float x, float y, float z) {
    m_cameraX = x;
    m_cameraY = y;
    m_cameraZ = z;
}

void ShadowCascadeSystem::setCameraFrustum(float fov, float aspect, float nearPlane, float farPlane) {
    m_cameraFov = fov;
    m_cameraAspect = aspect;
    m_cameraNear = nearPlane;
    m_cameraFar = farPlane;
    
    calculateCascadeSplits();
}

void ShadowCascadeSystem::update(float deltaTime) {
    m_stats.lastUpdateTime = deltaTime * 1000.0f; // Convert to ms
    m_stats.numCascades = static_cast<int>(m_cascades.size());
    m_stats.drawCalls = 0;
    
    updateCascadeMatrices();
}

void ShadowCascadeSystem::calculateCascadeSplits() {
    calculateSplitDistances();
    
    // Update cascade split ranges
    for (size_t i = 0; i < m_cascades.size(); ++i) {
        m_cascades[i].splitNear = (i == 0) ? m_cameraNear : m_splitDistances[i - 1];
        m_cascades[i].splitFar = m_splitDistances[i];
        m_cascades[i].splitDistance = m_splitDistances[i];
    }
}

void ShadowCascadeSystem::updateCascadeMatrices() {
    for (size_t i = 0; i < m_cascades.size(); ++i) {
        calculateCascadeMatrix(static_cast<int>(i));
        
        if (m_settings.stabilizeProjection) {
            stabilizeShadowMap(static_cast<int>(i));
        }
    }
}

void ShadowCascadeSystem::beginShadowPass(int cascadeIndex) {
    if (cascadeIndex < 0 || cascadeIndex >= static_cast<int>(m_cascades.size())) {
        return;
    }
    
    // Bind shadow framebuffer
    // glBindFramebuffer(GL_FRAMEBUFFER, m_cascades[cascadeIndex].framebuffer);
    
    // Clear depth
    // glClear(GL_DEPTH_BUFFER_BIT);
    
    // Set viewport
    int res = m_cascades[cascadeIndex].resolution;
    // glViewport(0, 0, res, res);
    
    m_stats.drawCalls++;
}

void ShadowCascadeSystem::endShadowPass() {
    // Unbind framebuffer
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int ShadowCascadeSystem::getShadowMapTexture(int cascadeIndex) const {
    if (cascadeIndex < 0 || cascadeIndex >= static_cast<int>(m_cascades.size())) {
        return 0;
    }
    return m_cascades[cascadeIndex].depthTexture;
}

int ShadowCascadeSystem::selectCascade(float depth) const {
    for (size_t i = 0; i < m_splitDistances.size(); ++i) {
        if (depth < m_splitDistances[i]) {
            return static_cast<int>(i);
        }
    }
    return static_cast<int>(m_splitDistances.size()) - 1;
}

float ShadowCascadeSystem::getCascadeBlendFactor(float depth, int cascadeIndex) const {
    if (cascadeIndex < 0 || cascadeIndex >= static_cast<int>(m_cascades.size())) {
        return 0.0f;
    }
    
    float splitDistance = m_splitDistances[cascadeIndex];
    float blendStart = splitDistance - m_settings.cascadeBlendDistance;
    
    if (depth < blendStart) {
        return 0.0f;
    }
    
    float blendFactor = (depth - blendStart) / m_settings.cascadeBlendDistance;
    return std::clamp(blendFactor, 0.0f, 1.0f);
}

float ShadowCascadeSystem::calculateAdaptiveBias(float distance) const {
    // Increase bias with distance to prevent shadow acne
    float baseBias = m_settings.bias;
    float distanceFactor = distance / m_cameraFar;
    return baseBias * (1.0f + distanceFactor * 10.0f);
}

void ShadowCascadeSystem::resetStats() {
    m_stats = ShadowStats();
    m_stats.numCascades = static_cast<int>(m_cascades.size());
    m_stats.totalResolution = m_stats.numCascades * m_settings.resolution * m_settings.resolution;
}

void ShadowCascadeSystem::createShadowMaps() {
    m_cascades.resize(m_settings.numCascades);
    
    for (int i = 0; i < m_settings.numCascades; ++i) {
        ShadowCascade& cascade = m_cascades[i];
        cascade.resolution = m_settings.resolution;
        
        // Create framebuffer
        // glGenFramebuffers(1, &cascade.framebuffer);
        // glBindFramebuffer(GL_FRAMEBUFFER, cascade.framebuffer);
        
        // Create depth texture
        // glGenTextures(1, &cascade.depthTexture);
        // glBindTexture(GL_TEXTURE_2D, cascade.depthTexture);
        // glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        
        // Attach depth texture
        // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, cascade.depthTexture, 0);
        
        // No color attachment
        // glDrawBuffer(GL_NONE);
        // glReadBuffer(GL_NONE);
        
        // Initialize matrices
        std::memset(cascade.viewMatrix, 0, sizeof(cascade.viewMatrix));
        std::memset(cascade.projMatrix, 0, sizeof(cascade.projMatrix));
        for (int j = 0; j < 4; ++j) {
            cascade.viewMatrix[j * 4 + j] = 1.0f;
            cascade.projMatrix[j * 4 + j] = 1.0f;
        }
    }
    
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    m_stats.totalResolution = m_settings.numCascades * m_settings.resolution * m_settings.resolution;
}

void ShadowCascadeSystem::destroyShadowMaps() {
    for (auto& cascade : m_cascades) {
        // glDeleteTextures(1, &cascade.depthTexture);
        // glDeleteFramebuffers(1, &cascade.framebuffer);
    }
    m_cascades.clear();
    m_splitDistances.clear();
}

void ShadowCascadeSystem::calculateSplitDistances() {
    m_splitDistances.resize(m_settings.numCascades);
    
    float n = m_settings.numCascades;
    
    for (int i = 0; i < m_settings.numCascades; ++i) {
        float fi = static_cast<float>(i + 1);
        
        // Logarithmic split
        float logSplit = calculateLogSplit(m_cameraNear, m_cameraFar, fi, n);
        
        // Uniform split
        float uniformSplit = calculateUniformSplit(m_cameraNear, m_cameraFar, fi, n);
        
        // Blend between log and uniform using lambda
        m_splitDistances[i] = m_settings.lambda * logSplit + (1.0f - m_settings.lambda) * uniformSplit;
    }
}

void ShadowCascadeSystem::calculateCascadeMatrix(int index) {
    if (index < 0 || index >= static_cast<int>(m_cascades.size())) {
        return;
    }
    
    ShadowCascade& cascade = m_cascades[index];
    
    // Calculate frustum corners for this cascade split
    float near = cascade.splitNear;
    float far = cascade.splitFar;
    
    float tanHalfFov = std::tan(m_cameraFov * 0.5f * 3.14159f / 180.0f);
    float nearHeight = 2.0f * tanHalfFov * near;
    float nearWidth = nearHeight * m_cameraAspect;
    float farHeight = 2.0f * tanHalfFov * far;
    float farWidth = farHeight * m_cameraAspect;
    
    // Calculate frustum center
    float centerX = m_cameraX + m_lightDirX * (near + far) * 0.5f;
    float centerY = m_cameraY + m_lightDirY * (near + far) * 0.5f;
    float centerZ = m_cameraZ + m_lightDirZ * (near + far) * 0.5f;
    
    // Calculate light view matrix
    // Look at frustum center from light direction
    float eyeX = centerX - m_lightDirX * 100.0f;
    float eyeY = centerY - m_lightDirY * 100.0f;
    float eyeZ = centerZ - m_lightDirZ * 100.0f;
    
    std::memcpy(cascade.viewMatrix, m_lightView, sizeof(cascade.viewMatrix));
    
    // Calculate orthographic projection
    float radius = std::max(std::max(nearWidth, nearHeight), std::max(farWidth, farHeight)) * 0.5f;
    float left = -radius;
    float right = radius;
    float bottom = -radius;
    float top = radius;
    float orthoNear = -radius * 10.0f;
    float orthoFar = radius * 10.0f;
    
    // Orthographic projection matrix
    cascade.projMatrix[0] = 2.0f / (right - left);
    cascade.projMatrix[5] = 2.0f / (top - bottom);
    cascade.projMatrix[10] = -2.0f / (orthoFar - orthoNear);
    cascade.projMatrix[12] = -(right + left) / (right - left);
    cascade.projMatrix[13] = -(top + bottom) / (top - bottom);
    cascade.projMatrix[14] = -(orthoFar + orthoNear) / (orthoFar - orthoNear);
    cascade.projMatrix[15] = 1.0f;
}

void ShadowCascadeSystem::stabilizeShadowMap(int index) {
    if (index < 0 || index >= static_cast<int>(m_cascades.size())) {
        return;
    }
    
    ShadowCascade& cascade = m_cascades[index];
    
    // Snap to texel grid to prevent shimmering
    float texelSize = 1.0f / static_cast<float>(cascade.resolution);
    
    // Round shadow map origin to texel grid
    float* proj = cascade.projMatrix;
    proj[12] = std::round(proj[12] / texelSize) * texelSize;
    proj[13] = std::round(proj[13] / texelSize) * texelSize;
}

float ShadowCascadeSystem::calculateLogSplit(float near, float far, float i, float n) const {
    return near * std::pow(far / near, i / n);
}

float ShadowCascadeSystem::calculateUniformSplit(float near, float far, float i, float n) const {
    return near + (far - near) * (i / n);
}

// ShadowSystem implementation
ShadowSystem& ShadowSystem::getInstance() {
    static ShadowSystem instance;
    return instance;
}

void ShadowSystem::initialize() {
    m_cascadeSystem.initialize();
}

void ShadowSystem::shutdown() {
    m_cascadeSystem.shutdown();
}

void ShadowSystem::setQuality(ShadowQuality quality) {
    m_cascadeSystem.setQuality(quality);
}

} // namespace Engine
