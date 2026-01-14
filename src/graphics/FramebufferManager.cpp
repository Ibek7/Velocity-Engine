#include "graphics/FramebufferManager.h"
#include <iostream>
#include <cstring>

namespace JJM {
namespace Graphics {

// FramebufferAttachment Implementation
FramebufferAttachment::FramebufferAttachment(const AttachmentConfig& config)
    : m_config(config)
    , m_textureId(0) {
    // TODO: Create texture based on config
}

FramebufferAttachment::~FramebufferAttachment() {
    // TODO: Delete texture
    if (m_textureId != 0) {
        // glDeleteTextures(1, &m_textureId);
    }
}

void FramebufferAttachment::resize(uint32_t width, uint32_t height) {
    // TODO: Resize texture
}

// Framebuffer Implementation
Framebuffer::Framebuffer(const std::string& name, const FramebufferSpec& spec)
    : m_name(name)
    , m_spec(spec)
    , m_fbo(0) {
    create();
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::bind() const {
    // TODO: Bind framebuffer
    // glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
}

void Framebuffer::unbind() const {
    // TODO: Unbind framebuffer
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

uint32_t Framebuffer::getColorAttachment(uint32_t index) const {
    for (const auto& attachment : m_attachments) {
        if (attachment->getType() == AttachmentType::Color && 
            attachment->getIndex() == index) {
            return attachment->getTextureId();
        }
    }
    return 0;
}

uint32_t Framebuffer::getDepthAttachment() const {
    for (const auto& attachment : m_attachments) {
        if (attachment->getType() == AttachmentType::Depth ||
            attachment->getType() == AttachmentType::DepthStencil) {
            return attachment->getTextureId();
        }
    }
    return 0;
}

void Framebuffer::clear(const float* clearColor) {
    bind();
    
    // TODO: Clear framebuffer
    // glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
    // glClear(GL_COLOR_BUFFER_BIT);
}

void Framebuffer::clearDepth(float depth) {
    bind();
    
    // TODO: Clear depth buffer
    // glClearDepth(depth);
    // glClear(GL_DEPTH_BUFFER_BIT);
}

void Framebuffer::clearStencil(int32_t stencil) {
    bind();
    
    // TODO: Clear stencil buffer
    // glClearStencil(stencil);
    // glClear(GL_STENCIL_BUFFER_BIT);
}

void Framebuffer::resize(uint32_t width, uint32_t height) {
    if (m_spec.width == width && m_spec.height == height) {
        return;
    }
    
    m_spec.width = width;
    m_spec.height = height;
    
    invalidate();
}

void Framebuffer::blitTo(const Framebuffer* target, bool color, bool depth, bool stencil) const {
    if (!target) return;
    
    // TODO: Blit framebuffer
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->getFBO());
    // glBlitFramebuffer(...);
}

void Framebuffer::readPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                            void* data, uint32_t attachmentIndex) const {
    bind();
    
    // TODO: Read pixels from framebuffer
    // glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
    // glReadPixels(x, y, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
}

bool Framebuffer::isComplete() const {
    // TODO: Check framebuffer completeness
    // return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    return true;
}

void Framebuffer::create() {
    // TODO: Create framebuffer object
    // glGenFramebuffers(1, &m_fbo);
    // glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    
    // Create attachments
    for (const auto& config : m_spec.attachments) {
        auto attachment = std::make_unique<FramebufferAttachment>(config);
        
        // TODO: Attach to framebuffer
        // uint32_t attachmentPoint = ...;
        // glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentPoint, GL_TEXTURE_2D, 
        //                       attachment->getTextureId(), 0);
        
        m_attachments.push_back(std::move(attachment));
    }
    
    // Check completeness
    if (!isComplete()) {
        std::cerr << "Framebuffer " << m_name << " is not complete!" << std::endl;
    }
}

void Framebuffer::destroy() {
    m_attachments.clear();
    
    if (m_fbo != 0) {
        // TODO: Delete framebuffer
        // glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
}

void Framebuffer::invalidate() {
    destroy();
    create();
}

// FramebufferManager Implementation
FramebufferManager::FramebufferManager()
    : m_nextId(1)
    , m_currentFBO(0) {
}

FramebufferManager::~FramebufferManager() {
    m_framebuffers.clear();
}

uint32_t FramebufferManager::createFramebuffer(const std::string& name, const FramebufferSpec& spec) {
    uint32_t id = m_nextId++;
    auto framebuffer = std::make_unique<Framebuffer>(name, spec);
    
    m_framebuffers[id] = std::move(framebuffer);
    m_nameMap[name] = id;
    m_stats.framebufferCount++;
    
    return id;
}

void FramebufferManager::destroyFramebuffer(uint32_t id) {
    auto it = m_framebuffers.find(id);
    if (it != m_framebuffers.end()) {
        m_nameMap.erase(it->second->getName());
        m_framebuffers.erase(it);
        m_stats.framebufferCount--;
    }
}

Framebuffer* FramebufferManager::getFramebuffer(uint32_t id) {
    auto it = m_framebuffers.find(id);
    return (it != m_framebuffers.end()) ? it->second.get() : nullptr;
}

const Framebuffer* FramebufferManager::getFramebuffer(uint32_t id) const {
    auto it = m_framebuffers.find(id);
    return (it != m_framebuffers.end()) ? it->second.get() : nullptr;
}

uint32_t FramebufferManager::findFramebuffer(const std::string& name) const {
    auto it = m_nameMap.find(name);
    return (it != m_nameMap.end()) ? it->second : 0;
}

void FramebufferManager::bindFramebuffer(uint32_t id) {
    Framebuffer* fbo = getFramebuffer(id);
    if (fbo) {
        fbo->bind();
        m_currentFBO = id;
        m_stats.drawCallsToFBO++;
    }
}

void FramebufferManager::unbindFramebuffer() {
    // TODO: Bind default framebuffer
    // glBindFramebuffer(GL_FRAMEBUFFER, 0);
    m_currentFBO = 0;
}

uint32_t FramebufferManager::createColorOnlyFBO(const std::string& name, 
                                               uint32_t width, uint32_t height,
                                               TextureFormat format) {
    FramebufferSpec spec;
    spec.width = width;
    spec.height = height;
    
    AttachmentConfig colorAttachment;
    colorAttachment.type = AttachmentType::Color;
    colorAttachment.format = format;
    colorAttachment.index = 0;
    
    spec.attachments.push_back(colorAttachment);
    
    return createFramebuffer(name, spec);
}

uint32_t FramebufferManager::createColorDepthFBO(const std::string& name,
                                                 uint32_t width, uint32_t height,
                                                 TextureFormat colorFormat,
                                                 TextureFormat depthFormat) {
    FramebufferSpec spec;
    spec.width = width;
    spec.height = height;
    
    AttachmentConfig colorAttachment;
    colorAttachment.type = AttachmentType::Color;
    colorAttachment.format = colorFormat;
    colorAttachment.index = 0;
    
    AttachmentConfig depthAttachment;
    depthAttachment.type = AttachmentType::Depth;
    depthAttachment.format = depthFormat;
    
    spec.attachments.push_back(colorAttachment);
    spec.attachments.push_back(depthAttachment);
    
    return createFramebuffer(name, spec);
}

uint32_t FramebufferManager::createMultisampledFBO(const std::string& name,
                                                   uint32_t width, uint32_t height,
                                                   uint32_t samples) {
    FramebufferSpec spec;
    spec.width = width;
    spec.height = height;
    
    AttachmentConfig colorAttachment;
    colorAttachment.type = AttachmentType::Color;
    colorAttachment.format = TextureFormat::RGBA8;
    colorAttachment.index = 0;
    colorAttachment.multisampled = true;
    colorAttachment.samples = samples;
    
    AttachmentConfig depthAttachment;
    depthAttachment.type = AttachmentType::Depth;
    depthAttachment.format = TextureFormat::Depth24;
    depthAttachment.multisampled = true;
    depthAttachment.samples = samples;
    
    spec.attachments.push_back(colorAttachment);
    spec.attachments.push_back(depthAttachment);
    
    return createFramebuffer(name, spec);
}

uint32_t FramebufferManager::createGBufferFBO(const std::string& name,
                                              uint32_t width, uint32_t height) {
    FramebufferSpec spec;
    spec.width = width;
    spec.height = height;
    
    // Position + depth
    AttachmentConfig positionAttachment;
    positionAttachment.type = AttachmentType::Color;
    positionAttachment.format = TextureFormat::RGBA16F;
    positionAttachment.index = 0;
    
    // Normal + roughness
    AttachmentConfig normalAttachment;
    normalAttachment.type = AttachmentType::Color;
    normalAttachment.format = TextureFormat::RGBA16F;
    normalAttachment.index = 1;
    
    // Albedo + metallic
    AttachmentConfig albedoAttachment;
    albedoAttachment.type = AttachmentType::Color;
    albedoAttachment.format = TextureFormat::RGBA8;
    albedoAttachment.index = 2;
    
    // Depth
    AttachmentConfig depthAttachment;
    depthAttachment.type = AttachmentType::Depth;
    depthAttachment.format = TextureFormat::Depth24;
    
    spec.attachments.push_back(positionAttachment);
    spec.attachments.push_back(normalAttachment);
    spec.attachments.push_back(albedoAttachment);
    spec.attachments.push_back(depthAttachment);
    
    return createFramebuffer(name, spec);
}

uint32_t FramebufferManager::createShadowMapFBO(const std::string& name, uint32_t resolution) {
    FramebufferSpec spec;
    spec.width = resolution;
    spec.height = resolution;
    
    AttachmentConfig depthAttachment;
    depthAttachment.type = AttachmentType::Depth;
    depthAttachment.format = TextureFormat::Depth32F;
    
    spec.attachments.push_back(depthAttachment);
    
    return createFramebuffer(name, spec);
}

void FramebufferManager::resizeAll(uint32_t width, uint32_t height) {
    for (auto& pair : m_framebuffers) {
        pair.second->resize(width, height);
    }
}

void FramebufferManager::resetStatistics() {
    m_stats.drawCallsToFBO = 0;
}

void FramebufferManager::dumpFramebufferInfo() const {
    std::cout << "=== Framebuffer Manager ===" << std::endl;
    std::cout << "Total Framebuffers: " << m_stats.framebufferCount << std::endl;
    std::cout << "Draw Calls to FBO: " << m_stats.drawCallsToFBO << std::endl;
    
    for (const auto& pair : m_framebuffers) {
        const Framebuffer* fbo = pair.second.get();
        std::cout << "  FBO " << pair.first << " (" << fbo->getName() << "): "
                  << fbo->getWidth() << "x" << fbo->getHeight()
                  << " - " << fbo->getAttachmentCount() << " attachments" << std::endl;
    }
}

// RenderTarget Implementation
RenderTarget::RenderTarget(Framebuffer* framebuffer)
    : m_framebuffer(framebuffer)
    , m_previousFBO(0)
    , m_bound(false) {
}

RenderTarget::~RenderTarget() {
    if (m_bound) {
        unbind();
    }
}

void RenderTarget::bind() {
    if (m_framebuffer && !m_bound) {
        // TODO: Get current FBO
        // glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&m_previousFBO);
        
        m_framebuffer->bind();
        m_bound = true;
    }
}

void RenderTarget::unbind() {
    if (m_bound) {
        // TODO: Restore previous FBO
        // glBindFramebuffer(GL_FRAMEBUFFER, m_previousFBO);
        m_bound = false;
    }
}

} // namespace Graphics
} // namespace JJM
