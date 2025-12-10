#pragma once

#include "graphics/Texture.h"
#include <memory>
#include <vector>

namespace JJM {
namespace Graphics {

enum class AttachmentType {
    Color,
    Depth,
    Stencil,
    DepthStencil
};

struct FramebufferAttachment {
    AttachmentType type;
    std::shared_ptr<Texture> texture;
    int mipmapLevel;
    
    FramebufferAttachment() 
        : type(AttachmentType::Color), texture(nullptr), mipmapLevel(0) {}
    
    FramebufferAttachment(AttachmentType type, std::shared_ptr<Texture> tex, int level = 0)
        : type(type), texture(tex), mipmapLevel(level) {}
};

class RenderTarget {
public:
    RenderTarget(int width, int height);
    ~RenderTarget();
    
    bool create();
    void destroy();
    
    void bind();
    void unbind();
    
    void attachTexture(AttachmentType type, std::shared_ptr<Texture> texture, int mipmapLevel = 0);
    void attachColorTexture(std::shared_ptr<Texture> texture, int colorAttachment = 0);
    void attachDepthTexture(std::shared_ptr<Texture> texture);
    
    std::shared_ptr<Texture> getColorTexture(int index = 0) const;
    std::shared_ptr<Texture> getDepthTexture() const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    bool isValid() const { return valid; }
    bool isComplete() const;
    
    void clear(float r, float g, float b, float a);
    void clearDepth(float depth = 1.0f);
    void clearStencil(int stencil = 0);
    
    void resize(int newWidth, int newHeight);
    
    unsigned int getFramebufferID() const { return framebufferID; }

private:
    int width;
    int height;
    unsigned int framebufferID;
    bool valid;
    
    std::vector<FramebufferAttachment> colorAttachments;
    FramebufferAttachment depthAttachment;
    FramebufferAttachment stencilAttachment;
};

class MultiSampleRenderTarget {
public:
    MultiSampleRenderTarget(int width, int height, int samples);
    ~MultiSampleRenderTarget();
    
    bool create();
    void destroy();
    
    void bind();
    void unbind();
    
    void resolve(RenderTarget& target);
    
    int getSamples() const { return samples; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

private:
    int width;
    int height;
    int samples;
    unsigned int framebufferID;
    unsigned int colorRenderbuffer;
    unsigned int depthRenderbuffer;
    bool valid;
};

class RenderTargetManager {
public:
    RenderTargetManager();
    ~RenderTargetManager();
    
    std::shared_ptr<RenderTarget> createRenderTarget(int width, int height);
    std::shared_ptr<RenderTarget> createRenderTarget(const std::string& name, int width, int height);
    
    std::shared_ptr<RenderTarget> getRenderTarget(const std::string& name);
    
    void destroyRenderTarget(const std::string& name);
    void destroyAllRenderTargets();
    
    void bindRenderTarget(const std::string& name);
    void unbindRenderTarget();
    
    std::shared_ptr<RenderTarget> getCurrentRenderTarget() { return currentTarget; }

private:
    std::vector<std::shared_ptr<RenderTarget>> renderTargets;
    std::shared_ptr<RenderTarget> currentTarget;
};

class FramebufferObject {
public:
    FramebufferObject();
    ~FramebufferObject();
    
    bool init(int width, int height, bool withDepth = true, bool withStencil = false);
    void cleanup();
    
    void bind() const;
    void unbind() const;
    
    unsigned int getTextureID() const { return textureID; }
    unsigned int getDepthTextureID() const { return depthTextureID; }
    unsigned int getFBO() const { return fbo; }
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
    bool isValid() const { return valid; }

private:
    unsigned int fbo;
    unsigned int textureID;
    unsigned int depthTextureID;
    unsigned int depthRenderbuffer;
    unsigned int stencilRenderbuffer;
    
    int width;
    int height;
    bool valid;
    bool hasDepth;
    bool hasStencil;
};

} // namespace Graphics
} // namespace JJM
