#include "graphics/DeferredRendering.h"
#include <cstring>
#include <algorithm>
#include <random>

namespace Engine {

DeferredRenderer::DeferredRenderer()
    : m_gBufferFBO(0)
    , m_albedoTexture(0)
    , m_normalTexture(0)
    , m_positionTexture(0)
    , m_metallicRoughnessTexture(0)
    , m_emissionTexture(0)
    , m_depthTexture(0)
    , m_ssaoFBO(0)
    , m_ssaoTexture(0)
    , m_ssaoBlurFBO(0)
    , m_ssaoBlurTexture(0)
    , m_ssaoNoiseTexture(0)
    , m_nextLightId(0)
    , m_geometryShader(0)
    , m_lightingShader(0)
    , m_directionalLightShader(0)
    , m_pointLightShader(0)
    , m_spotLightShader(0)
    , m_ssaoShader(0)
    , m_ssaoBlurShader(0)
    , m_debugVisualization(false)
    , m_debugTexture(GBufferTexture::Albedo)
{
}

DeferredRenderer::~DeferredRenderer() {
    shutdown();
}

void DeferredRenderer::initialize(int width, int height) {
    m_settings.width = width;
    m_settings.height = height;
    
    createGBuffer();
    setupGeometryShader();
    setupLightingShader();
    
    if (m_settings.useSSAO) {
        generateSSAOKernel();
        generateSSAONoise();
    }
}

void DeferredRenderer::shutdown() {
    destroyGBuffer();
    clearLights();
}

void DeferredRenderer::resize(int width, int height) {
    m_settings.width = width;
    m_settings.height = height;
    
    destroyGBuffer();
    createGBuffer();
}

void DeferredRenderer::setSettings(const GBufferSettings& settings) {
    bool needResize = (settings.width != m_settings.width || 
                       settings.height != m_settings.height);
    
    m_settings = settings;
    
    if (needResize) {
        resize(settings.width, settings.height);
    }
    
    if (settings.useSSAO && m_ssaoKernel.empty()) {
        generateSSAOKernel();
        generateSSAONoise();
    }
}

void DeferredRenderer::createGBuffer() {
    // Create framebuffer
    // glGenFramebuffers(1, &m_gBufferFBO);
    // glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);
    
    createGBufferTexture(GBufferTexture::Albedo);
    createGBufferTexture(GBufferTexture::Normal);
    createGBufferTexture(GBufferTexture::Position);
    createGBufferTexture(GBufferTexture::MetallicRoughness);
    createGBufferTexture(GBufferTexture::Emission);
    createGBufferTexture(GBufferTexture::Depth);
    
    // Attach textures to framebuffer
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_albedoTexture, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTexture, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_positionTexture, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_metallicRoughnessTexture, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, m_emissionTexture, 0);
    // glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Set draw buffers
    // unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    // glDrawBuffers(5, attachments);
    
    // Check framebuffer status
    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    //     // Error handling
    // }
    
    // Create SSAO buffers
    if (m_settings.useSSAO) {
        // glGenFramebuffers(1, &m_ssaoFBO);
        // glGenTextures(1, &m_ssaoTexture);
        // glGenFramebuffers(1, &m_ssaoBlurFBO);
        // glGenTextures(1, &m_ssaoBlurTexture);
    }
    
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::destroyGBuffer() {
    // glDeleteFramebuffers(1, &m_gBufferFBO);
    // glDeleteTextures(1, &m_albedoTexture);
    // glDeleteTextures(1, &m_normalTexture);
    // glDeleteTextures(1, &m_positionTexture);
    // glDeleteTextures(1, &m_metallicRoughnessTexture);
    // glDeleteTextures(1, &m_emissionTexture);
    // glDeleteTextures(1, &m_depthTexture);
    
    if (m_settings.useSSAO) {
        // glDeleteFramebuffers(1, &m_ssaoFBO);
        // glDeleteTextures(1, &m_ssaoTexture);
        // glDeleteFramebuffers(1, &m_ssaoBlurFBO);
        // glDeleteTextures(1, &m_ssaoBlurTexture);
        // glDeleteTextures(1, &m_ssaoNoiseTexture);
    }
}

unsigned int DeferredRenderer::getGBufferTexture(GBufferTexture type) const {
    switch (type) {
        case GBufferTexture::Albedo: return m_albedoTexture;
        case GBufferTexture::Normal: return m_normalTexture;
        case GBufferTexture::Position: return m_positionTexture;
        case GBufferTexture::MetallicRoughness: return m_metallicRoughnessTexture;
        case GBufferTexture::Emission: return m_emissionTexture;
        case GBufferTexture::Depth: return m_depthTexture;
        default: return 0;
    }
}

void DeferredRenderer::beginGeometryPass() {
    // glBindFramebuffer(GL_FRAMEBUFFER, m_gBufferFBO);
    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glEnable(GL_DEPTH_TEST);
    // glUseProgram(m_geometryShader);
    
    m_stats.geometryDrawCalls = 0;
}

void DeferredRenderer::endGeometryPass() {
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeferredRenderer::beginLightingPass() {
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glClear(GL_COLOR_BUFFER_BIT);
    // glDisable(GL_DEPTH_TEST);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_ONE, GL_ONE); // Additive blending
    
    // Bind G-Buffer textures
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, m_albedoTexture);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, m_normalTexture);
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, m_positionTexture);
    // glActiveTexture(GL_TEXTURE3);
    // glBindTexture(GL_TEXTURE_2D, m_metallicRoughnessTexture);
    
    if (m_settings.useSSAO) {
        renderSSAO();
        // glActiveTexture(GL_TEXTURE4);
        // glBindTexture(GL_TEXTURE_2D, m_ssaoBlurTexture);
    }
}

void DeferredRenderer::endLightingPass() {
    // glDisable(GL_BLEND);
    // glEnable(GL_DEPTH_TEST);
}

int DeferredRenderer::addLight(const DeferredLight& light) {
    int id = m_nextLightId++;
    m_lights.push_back(light);
    m_stats.lightCount = static_cast<int>(m_lights.size());
    return id;
}

void DeferredRenderer::removeLight(int lightId) {
    if (lightId >= 0 && lightId < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + lightId);
        m_stats.lightCount = static_cast<int>(m_lights.size());
    }
}

void DeferredRenderer::updateLight(int lightId, const DeferredLight& light) {
    if (lightId >= 0 && lightId < static_cast<int>(m_lights.size())) {
        m_lights[lightId] = light;
    }
}

DeferredLight* DeferredRenderer::getLight(int lightId) {
    if (lightId >= 0 && lightId < static_cast<int>(m_lights.size())) {
        return &m_lights[lightId];
    }
    return nullptr;
}

void DeferredRenderer::clearLights() {
    m_lights.clear();
    m_visibleLights.clear();
    m_stats.lightCount = 0;
    m_stats.visibleLights = 0;
}

void DeferredRenderer::cullLights(const float viewProj[16]) {
    m_visibleLights.clear();
    
    for (size_t i = 0; i < m_lights.size(); ++i) {
        if (isLightVisible(m_lights[i], viewProj)) {
            m_visibleLights.push_back(static_cast<int>(i));
        }
    }
    
    m_stats.visibleLights = static_cast<int>(m_visibleLights.size());
}

void DeferredRenderer::enableSSAO(bool enable) {
    m_settings.useSSAO = enable;
}

void DeferredRenderer::generateSSAOKernel() {
    m_ssaoKernel.clear();
    m_ssaoKernel.reserve(m_settings.ssaoSamples * 3);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    
    for (int i = 0; i < m_settings.ssaoSamples; ++i) {
        // Generate random sample in hemisphere
        float x = randomFloats(gen) * 2.0f - 1.0f;
        float y = randomFloats(gen) * 2.0f - 1.0f;
        float z = randomFloats(gen);
        
        // Normalize
        float length = std::sqrt(x * x + y * y + z * z);
        x /= length;
        y /= length;
        z /= length;
        
        // Scale by random factor (more samples closer to origin)
        float scale = static_cast<float>(i) / static_cast<float>(m_settings.ssaoSamples);
        scale = 0.1f + scale * scale * 0.9f; // Lerp between 0.1 and 1.0
        
        m_ssaoKernel.push_back(x * scale);
        m_ssaoKernel.push_back(y * scale);
        m_ssaoKernel.push_back(z * scale);
    }
}

void DeferredRenderer::generateSSAONoise() {
    std::vector<float> ssaoNoise;
    ssaoNoise.reserve(16 * 3); // 4x4 noise texture
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    
    for (int i = 0; i < 16; ++i) {
        // Random rotation vector
        float x = randomFloats(gen) * 2.0f - 1.0f;
        float y = randomFloats(gen) * 2.0f - 1.0f;
        float z = 0.0f; // Rotate around Z axis
        
        ssaoNoise.push_back(x);
        ssaoNoise.push_back(y);
        ssaoNoise.push_back(z);
    }
    
    // Create noise texture
    // glGenTextures(1, &m_ssaoNoiseTexture);
    // glBindTexture(GL_TEXTURE_2D, m_ssaoNoiseTexture);
    // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void DeferredRenderer::renderSSAO() {
    // Render SSAO to texture
    // glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoFBO);
    // glClear(GL_COLOR_BUFFER_BIT);
    // glUseProgram(m_ssaoShader);
    
    // Bind position and normal textures
    // Set kernel samples and noise texture
    
    // Render full screen quad
    
    // Blur SSAO
    // glBindFramebuffer(GL_FRAMEBUFFER, m_ssaoBlurFBO);
    // glClear(GL_COLOR_BUFFER_BIT);
    // glUseProgram(m_ssaoBlurShader);
    // glBindTexture(GL_TEXTURE_2D, m_ssaoTexture);
    
    // Render full screen quad
}

void DeferredRenderer::renderDirectionalLight(const DeferredLight& light) {
    // glUseProgram(m_directionalLightShader);
    
    // Set light uniforms
    // glUniform3f(directionLoc, light.directionX, light.directionY, light.directionZ);
    // glUniform3f(colorLoc, light.colorR, light.colorG, light.colorB);
    // glUniform1f(intensityLoc, light.intensity);
    
    // Render full screen quad
}

void DeferredRenderer::renderPointLight(const DeferredLight& light) {
    // glUseProgram(m_pointLightShader);
    
    // Set light uniforms
    // glUniform3f(positionLoc, light.positionX, light.positionY, light.positionZ);
    // glUniform3f(colorLoc, light.colorR, light.colorG, light.colorB);
    // glUniform1f(intensityLoc, light.intensity);
    // glUniform1f(rangeLoc, light.range);
    
    // Render light volume (sphere)
}

void DeferredRenderer::renderSpotLight(const DeferredLight& light) {
    // glUseProgram(m_spotLightShader);
    
    // Set light uniforms
    // Render light volume (cone)
}

void DeferredRenderer::renderDebugView() {
    // Render selected G-Buffer texture to screen
}

void DeferredRenderer::resetStats() {
    m_stats = DeferredStats();
    m_stats.lightCount = static_cast<int>(m_lights.size());
}

void DeferredRenderer::createGBufferTexture(GBufferTexture type) {
    unsigned int* texture = nullptr;
    // unsigned int format = GL_RGBA16F; // HDR format
    // unsigned int internalFormat = GL_RGBA;
    // unsigned int dataType = GL_FLOAT;
    
    switch (type) {
        case GBufferTexture::Albedo:
            texture = &m_albedoTexture;
            break;
        case GBufferTexture::Normal:
            texture = &m_normalTexture;
            break;
        case GBufferTexture::Position:
            texture = &m_positionTexture;
            break;
        case GBufferTexture::MetallicRoughness:
            texture = &m_metallicRoughnessTexture;
            break;
        case GBufferTexture::Emission:
            texture = &m_emissionTexture;
            break;
        case GBufferTexture::Depth:
            texture = &m_depthTexture;
            // format = GL_DEPTH_COMPONENT32F;
            // internalFormat = GL_DEPTH_COMPONENT;
            break;
    }
    
    if (texture) {
        // glGenTextures(1, texture);
        // glBindTexture(GL_TEXTURE_2D, *texture);
        // glTexImage2D(GL_TEXTURE_2D, 0, format, m_settings.width, m_settings.height, 0, internalFormat, dataType, nullptr);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
}

void DeferredRenderer::setupGeometryShader() {
    // Load and compile geometry shader
    // Shader outputs: albedo, normal, position, metallic-roughness, emission
}

void DeferredRenderer::setupLightingShader() {
    // Load and compile lighting shaders for each light type
}

void DeferredRenderer::calculateLightBounds(const DeferredLight& light, float& radius) const {
    if (light.type == LightType::Point) {
        radius = light.range;
    } else if (light.type == LightType::Spot) {
        radius = light.range;
    } else {
        radius = 0.0f;
    }
}

bool DeferredRenderer::isLightVisible(const DeferredLight& light, const float viewProj[16]) const {
    // For directional lights, always visible
    if (light.type == LightType::Directional) {
        return true;
    }
    
    // For point/spot lights, perform frustum culling
    float radius;
    calculateLightBounds(light, radius);
    
    // Simplified visibility check
    return true;
}

// DeferredRenderingSystem implementation
DeferredRenderingSystem& DeferredRenderingSystem::getInstance() {
    static DeferredRenderingSystem instance;
    return instance;
}

void DeferredRenderingSystem::initialize(int width, int height) {
    m_renderer.initialize(width, height);
}

void DeferredRenderingSystem::shutdown() {
    m_renderer.shutdown();
}

} // namespace Engine
