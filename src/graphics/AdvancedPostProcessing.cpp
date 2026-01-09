#include "graphics/AdvancedPostProcessing.h"
#include <GL/glew.h>
#include <cmath>

namespace JJM {
namespace Graphics {

// =============================================================================
// AdvancedBloom Implementation
// =============================================================================

AdvancedBloom::AdvancedBloom()
    : m_threshold(1.0f)
    , m_knee(0.1f)
    , m_intensity(0.8f)
    , m_mipLevels(6)
{}

AdvancedBloom::~AdvancedBloom() {
    shutdown();
}

void AdvancedBloom::initialize(int width, int height) {
    // TODO: Create shaders
    // m_downsampleShader = create shader...
    // m_upsampleShader = create shader...
    // m_thresholdShader = create shader...
    
    createMipChain(width, height);
}

void AdvancedBloom::shutdown() {
    for (unsigned int texture : m_mipChain) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
    }
    m_mipChain.clear();
}

void AdvancedBloom::render(unsigned int inputHDR, unsigned int output) {
    if (m_mipChain.empty()) return;
    
    // 1. Threshold pass - extract bright pixels
    if (m_thresholdShader) {
        // TODO: Apply threshold shader
        // Set uniforms: threshold, knee
    }
    
    // 2. Downsample pass - create mip chain
    for (size_t i = 1; i < m_mipChain.size(); ++i) {
        downsamplePass(m_mipChain[i - 1], m_mipChain[i]);
    }
    
    // 3. Upsample pass - blur and combine
    for (int i = static_cast<int>(m_mipChain.size()) - 2; i >= 0; --i) {
        upsamplePass(m_mipChain[i + 1], m_mipChain[i]);
    }
    
    // 4. Composite with original
    // TODO: Blend bloom result with input
}

void AdvancedBloom::setThreshold(float threshold, float knee) {
    m_threshold = threshold;
    m_knee = knee;
}

void AdvancedBloom::createMipChain(int width, int height) {
    m_mipChain.clear();
    
    for (int i = 0; i < m_mipLevels; ++i) {
        int mipWidth = std::max(1, width >> i);
        int mipHeight = std::max(1, height >> i);
        
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, mipWidth, mipHeight,
                    0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        m_mipChain.push_back(texture);
    }
}

void AdvancedBloom::downsamplePass(unsigned int source, unsigned int dest) {
    // TODO: Implement 13-tap Karis downsample filter
    // This prevents fireflies and flickering in bloom
    glBindTexture(GL_TEXTURE_2D, source);
    // Render to dest with downsample shader
}

void AdvancedBloom::upsamplePass(unsigned int source, unsigned int dest) {
    // TODO: Implement tent filter upsample
    // Provides smooth blur without artifacts
    glBindTexture(GL_TEXTURE_2D, source);
    // Render to dest with upsample shader
}

// =============================================================================
// ACESToneMapping Implementation
// =============================================================================

ACESToneMapping::ACESToneMapping()
    : m_exposure(1.0f)
{}

ACESToneMapping::~ACESToneMapping() {
    shutdown();
}

void ACESToneMapping::initialize() {
    // TODO: Create ACES tone mapping shader
}

void ACESToneMapping::shutdown() {
    m_shader.reset();
}

void ACESToneMapping::render(unsigned int inputHDR, unsigned int outputLDR) {
    if (!m_shader) return;
    
    // TODO: Apply ACES tone mapping
    // Set uniforms: exposure
    // Apply ACES film curve
}

float ACESToneMapping::ACESFilmRec2020(float x) {
    // ACES filmic tone mapping curve (approximation)
    // Stephen Hill (@self_shadow)
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

// =============================================================================
// LUTColorGrading Implementation
// =============================================================================

LUTColorGrading::LUTColorGrading()
    : m_lutTexture(0)
    , m_intensity(1.0f)
{}

LUTColorGrading::~LUTColorGrading() {
    shutdown();
}

void LUTColorGrading::initialize() {
    // TODO: Create color grading shader
}

void LUTColorGrading::shutdown() {
    if (m_lutTexture != 0) {
        glDeleteTextures(1, &m_lutTexture);
        m_lutTexture = 0;
    }
    m_shader.reset();
}

void LUTColorGrading::render(unsigned int input, unsigned int output) {
    if (!m_shader || m_lutTexture == 0) return;
    
    // TODO: Apply LUT color grading
    // Bind LUT texture to texture unit 1
    // Set uniforms: intensity
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, m_lutTexture);
}

void LUTColorGrading::loadLUT(const std::string& filepath) {
    // TODO: Load LUT from file (typically 32x32x32 or 64x64x64)
    // Support common formats: .cube, .3dl, .png strip
}

unsigned int LUTColorGrading::createLUTTexture(const unsigned char* data, int size) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB16F, size, size, size,
                0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    return texture;
}

// =============================================================================
// AdvancedPostProcessStack Implementation
// =============================================================================

AdvancedPostProcessStack::AdvancedPostProcessStack()
    : m_hdrTarget1(0)
    , m_hdrTarget2(0)
    , m_ldrTarget(0)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_bloomEnabled(true)
    , m_toneMappingEnabled(true)
    , m_colorGradingEnabled(false)
{
    m_bloom = std::make_unique<AdvancedBloom>();
    m_toneMapping = std::make_unique<ACESToneMapping>();
    m_colorGrading = std::make_unique<LUTColorGrading>();
}

AdvancedPostProcessStack::~AdvancedPostProcessStack() {
    shutdown();
}

void AdvancedPostProcessStack::initialize(int width, int height) {
    m_width = width;
    m_height = height;
    
    // Initialize effects
    if (m_bloom) {
        m_bloom->initialize(width, height);
    }
    if (m_toneMapping) {
        m_toneMapping->initialize();
    }
    if (m_colorGrading) {
        m_colorGrading->initialize();
    }
    
    // Create render targets
    createRenderTargets();
    
    m_initialized = true;
}

void AdvancedPostProcessStack::shutdown() {
    if (m_bloom) {
        m_bloom->shutdown();
    }
    if (m_toneMapping) {
        m_toneMapping->shutdown();
    }
    if (m_colorGrading) {
        m_colorGrading->shutdown();
    }
    
    destroyRenderTargets();
    m_initialized = false;
}

void AdvancedPostProcessStack::resize(int width, int height) {
    if (width == m_width && height == m_height) {
        return;
    }
    
    destroyRenderTargets();
    m_width = width;
    m_height = height;
    createRenderTargets();
    
    // Reinitialize bloom with new dimensions
    if (m_bloom) {
        m_bloom->shutdown();
        m_bloom->initialize(width, height);
    }
}

void AdvancedPostProcessStack::process(unsigned int inputHDR, unsigned int outputLDR) {
    if (!m_initialized) return;
    
    unsigned int currentInput = inputHDR;
    unsigned int currentOutput = m_hdrTarget1;
    
    // 1. Apply bloom (HDR -> HDR)
    if (m_bloomEnabled && m_bloom) {
        m_bloom->render(currentInput, currentOutput);
        std::swap(currentInput, currentOutput);
    }
    
    // 2. Apply tone mapping (HDR -> LDR)
    if (m_toneMappingEnabled && m_toneMapping) {
        m_toneMapping->render(currentInput, m_ldrTarget);
        currentInput = m_ldrTarget;
    }
    
    // 3. Apply color grading (LDR -> LDR)
    if (m_colorGradingEnabled && m_colorGrading) {
        m_colorGrading->render(currentInput, outputLDR);
    } else {
        // Copy to output if no color grading
        // TODO: Implement blit/copy
    }
}

void AdvancedPostProcessStack::createRenderTargets() {
    // HDR targets (RGBA16F)
    glGenTextures(1, &m_hdrTarget1);
    glBindTexture(GL_TEXTURE_2D, m_hdrTarget1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height,
                0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glGenTextures(1, &m_hdrTarget2);
    glBindTexture(GL_TEXTURE_2D, m_hdrTarget2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_width, m_height,
                0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // LDR target (RGBA8)
    glGenTextures(1, &m_ldrTarget);
    glBindTexture(GL_TEXTURE_2D, m_ldrTarget);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height,
                0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void AdvancedPostProcessStack::destroyRenderTargets() {
    if (m_hdrTarget1 != 0) {
        glDeleteTextures(1, &m_hdrTarget1);
        m_hdrTarget1 = 0;
    }
    if (m_hdrTarget2 != 0) {
        glDeleteTextures(1, &m_hdrTarget2);
        m_hdrTarget2 = 0;
    }
    if (m_ldrTarget != 0) {
        glDeleteTextures(1, &m_ldrTarget);
        m_ldrTarget = 0;
    }
}

} // namespace Graphics
} // namespace JJM
