#include "graphics/RenderTarget.h"
#include <iostream>

namespace JJM {
namespace Graphics {

// RenderTarget implementation
RenderTarget::RenderTarget(int width, int height)
    : width(width), height(height), framebufferID(0), valid(false) {}

RenderTarget::~RenderTarget() {
    destroy();
}

bool RenderTarget::create() {
    if (valid) {
        destroy();
    }
    
    // Platform-specific FBO creation would go here
    // This is a stub implementation
    framebufferID = 1; // Placeholder
    valid = true;
    
    return valid;
}

void RenderTarget::destroy() {
    if (framebufferID != 0) {
        // Platform-specific FBO deletion would go here
        framebufferID = 0;
    }
    
    colorAttachments.clear();
    depthAttachment = FramebufferAttachment();
    stencilAttachment = FramebufferAttachment();
    valid = false;
}

void RenderTarget::bind() {
    if (!valid) {
        std::cerr << "Attempting to bind invalid render target" << std::endl;
        return;
    }
    
    // Platform-specific binding would go here
}

void RenderTarget::unbind() {
    // Platform-specific unbinding would go here
}

void RenderTarget::attachTexture(AttachmentType type, std::shared_ptr<Texture> texture, int mipmapLevel) {
    FramebufferAttachment attachment(type, texture, mipmapLevel);
    
    switch (type) {
        case AttachmentType::Color:
            colorAttachments.push_back(attachment);
            break;
        case AttachmentType::Depth:
        case AttachmentType::DepthStencil:
            depthAttachment = attachment;
            break;
        case AttachmentType::Stencil:
            stencilAttachment = attachment;
            break;
    }
}

void RenderTarget::attachColorTexture(std::shared_ptr<Texture> texture, int colorAttachment) {
    if (colorAttachment >= static_cast<int>(colorAttachments.size())) {
        colorAttachments.resize(colorAttachment + 1);
    }
    
    colorAttachments[colorAttachment] = FramebufferAttachment(AttachmentType::Color, texture, 0);
}

void RenderTarget::attachDepthTexture(std::shared_ptr<Texture> texture) {
    depthAttachment = FramebufferAttachment(AttachmentType::Depth, texture, 0);
}

std::shared_ptr<Texture> RenderTarget::getColorTexture(int index) const {
    if (index < static_cast<int>(colorAttachments.size())) {
        return colorAttachments[index].texture;
    }
    return nullptr;
}

std::shared_ptr<Texture> RenderTarget::getDepthTexture() const {
    return depthAttachment.texture;
}

bool RenderTarget::isComplete() const {
    if (!valid) return false;
    
    // Check if at least one attachment exists
    return !colorAttachments.empty() || depthAttachment.texture != nullptr;
}

void RenderTarget::clear(float r, float g, float b, float a) {
    // Platform-specific clear would go here
}

void RenderTarget::clearDepth(float depth) {
    // Platform-specific depth clear would go here
}

void RenderTarget::clearStencil(int stencil) {
    // Platform-specific stencil clear would go here
}

void RenderTarget::resize(int newWidth, int newHeight) {
    if (newWidth == width && newHeight == height) return;
    
    width = newWidth;
    height = newHeight;
    
    // Recreate framebuffer with new dimensions
    if (valid) {
        destroy();
        create();
    }
}

// MultiSampleRenderTarget implementation
MultiSampleRenderTarget::MultiSampleRenderTarget(int width, int height, int samples)
    : width(width), height(height), samples(samples),
      framebufferID(0), colorRenderbuffer(0), depthRenderbuffer(0), valid(false) {}

MultiSampleRenderTarget::~MultiSampleRenderTarget() {
    destroy();
}

bool MultiSampleRenderTarget::create() {
    if (valid) {
        destroy();
    }
    
    // Platform-specific MSAA FBO creation would go here
    framebufferID = 1; // Placeholder
    colorRenderbuffer = 1; // Placeholder
    depthRenderbuffer = 1; // Placeholder
    valid = true;
    
    return valid;
}

void MultiSampleRenderTarget::destroy() {
    if (framebufferID != 0) {
        // Platform-specific deletion would go here
        framebufferID = 0;
        colorRenderbuffer = 0;
        depthRenderbuffer = 0;
    }
    valid = false;
}

void MultiSampleRenderTarget::bind() {
    if (!valid) {
        std::cerr << "Attempting to bind invalid MSAA render target" << std::endl;
        return;
    }
    
    // Platform-specific binding would go here
}

void MultiSampleRenderTarget::unbind() {
    // Platform-specific unbinding would go here
}

void MultiSampleRenderTarget::resolve(RenderTarget& target) {
    // Platform-specific MSAA resolve would go here
    // This blits the multisampled buffer to the regular target
}

// RenderTargetManager implementation
RenderTargetManager::RenderTargetManager() : currentTarget(nullptr) {}

RenderTargetManager::~RenderTargetManager() {
    destroyAllRenderTargets();
}

std::shared_ptr<RenderTarget> RenderTargetManager::createRenderTarget(int width, int height) {
    auto target = std::make_shared<RenderTarget>(width, height);
    target->create();
    renderTargets.push_back(target);
    return target;
}

std::shared_ptr<RenderTarget> RenderTargetManager::createRenderTarget(const std::string& name, int width, int height) {
    return createRenderTarget(width, height);
}

std::shared_ptr<RenderTarget> RenderTargetManager::getRenderTarget(const std::string& name) {
    // In a full implementation, would use a map with names
    return nullptr;
}

void RenderTargetManager::destroyRenderTarget(const std::string& name) {
    // In a full implementation, would find and remove by name
}

void RenderTargetManager::destroyAllRenderTargets() {
    for (auto& target : renderTargets) {
        target->destroy();
    }
    renderTargets.clear();
    currentTarget = nullptr;
}

void RenderTargetManager::bindRenderTarget(const std::string& name) {
    auto target = getRenderTarget(name);
    if (target) {
        target->bind();
        currentTarget = target;
    }
}

void RenderTargetManager::unbindRenderTarget() {
    if (currentTarget) {
        currentTarget->unbind();
        currentTarget = nullptr;
    }
}

// FramebufferObject implementation
FramebufferObject::FramebufferObject()
    : fbo(0), textureID(0), depthTextureID(0),
      depthRenderbuffer(0), stencilRenderbuffer(0),
      width(0), height(0), valid(false),
      hasDepth(false), hasStencil(false) {}

FramebufferObject::~FramebufferObject() {
    cleanup();
}

bool FramebufferObject::init(int width, int height, bool withDepth, bool withStencil) {
    this->width = width;
    this->height = height;
    this->hasDepth = withDepth;
    this->hasStencil = withStencil;
    
    // Platform-specific FBO initialization would go here
    fbo = 1; // Placeholder
    textureID = 1; // Placeholder
    
    if (withDepth) {
        depthTextureID = 1; // Placeholder
    }
    
    if (withStencil) {
        stencilRenderbuffer = 1; // Placeholder
    }
    
    valid = true;
    return valid;
}

void FramebufferObject::cleanup() {
    if (fbo != 0) {
        // Platform-specific cleanup would go here
        fbo = 0;
        textureID = 0;
        depthTextureID = 0;
        depthRenderbuffer = 0;
        stencilRenderbuffer = 0;
    }
    valid = false;
}

void FramebufferObject::bind() const {
    if (!valid) {
        std::cerr << "Attempting to bind invalid FBO" << std::endl;
        return;
    }
    
    // Platform-specific binding would go here
}

void FramebufferObject::unbind() const {
    // Platform-specific unbinding would go here
}

} // namespace Graphics
} // namespace JJM
