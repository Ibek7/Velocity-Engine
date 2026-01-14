#ifndef FRAMEBUFFER_MANAGER_H
#define FRAMEBUFFER_MANAGER_H

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace JJM {
namespace Graphics {

/**
 * @brief Framebuffer attachment type
 */
enum class AttachmentType {
    Color,
    Depth,
    Stencil,
    DepthStencil
};

/**
 * @brief Texture format for attachments
 */
enum class TextureFormat {
    // Color formats
    RGBA8,
    RGBA16F,
    RGBA32F,
    RGB8,
    RGB16F,
    RGB32F,
    RG8,
    RG16F,
    RG32F,
    R8,
    R16F,
    R32F,
    
    // Depth formats
    Depth16,
    Depth24,
    Depth32F,
    
    // Depth-stencil formats
    Depth24Stencil8,
    Depth32FStencil8,
    
    // Special formats
    SRGBA8,
    R11G11B10F
};

/**
 * @brief Framebuffer attachment configuration
 */
struct AttachmentConfig {
    AttachmentType type;
    TextureFormat format;
    uint32_t index;              // Attachment index (for multiple color attachments)
    bool multisampled;
    uint32_t samples;
    
    AttachmentConfig()
        : type(AttachmentType::Color)
        , format(TextureFormat::RGBA8)
        , index(0)
        , multisampled(false)
        , samples(1) {}
};

/**
 * @brief Framebuffer specification
 */
struct FramebufferSpec {
    uint32_t width;
    uint32_t height;
    std::vector<AttachmentConfig> attachments;
    bool swapChainTarget;        // True if this is for presentation
    uint32_t layers;             // For layered framebuffers (cubemaps, arrays)
    
    FramebufferSpec()
        : width(1280)
        , height(720)
        , swapChainTarget(false)
        , layers(1) {}
};

/**
 * @brief Framebuffer attachment
 */
class FramebufferAttachment {
public:
    FramebufferAttachment(const AttachmentConfig& config);
    ~FramebufferAttachment();
    
    AttachmentType getType() const { return m_config.type; }
    TextureFormat getFormat() const { return m_config.format; }
    uint32_t getTextureId() const { return m_textureId; }
    uint32_t getIndex() const { return m_config.index; }
    
    void resize(uint32_t width, uint32_t height);
    
private:
    AttachmentConfig m_config;
    uint32_t m_textureId;
};

/**
 * @brief Framebuffer object
 */
class Framebuffer {
public:
    Framebuffer(const std::string& name, const FramebufferSpec& spec);
    ~Framebuffer();
    
    const std::string& getName() const { return m_name; }
    uint32_t getWidth() const { return m_spec.width; }
    uint32_t getHeight() const { return m_spec.height; }
    uint32_t getFBO() const { return m_fbo; }
    
    // Binding
    void bind() const;
    void unbind() const;
    
    // Attachments
    uint32_t getColorAttachment(uint32_t index = 0) const;
    uint32_t getDepthAttachment() const;
    uint32_t getAttachmentCount() const { return static_cast<uint32_t>(m_attachments.size()); }
    
    // Operations
    void clear(const float* clearColor = nullptr);
    void clearDepth(float depth = 1.0f);
    void clearStencil(int32_t stencil = 0);
    
    void resize(uint32_t width, uint32_t height);
    
    // Blitting (copy from one framebuffer to another)
    void blitTo(const Framebuffer* target, bool color, bool depth, bool stencil) const;
    
    // Readback
    void readPixels(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                   void* data, uint32_t attachmentIndex = 0) const;
    
    // Validation
    bool isComplete() const;
    
private:
    void create();
    void destroy();
    void invalidate();
    
    std::string m_name;
    FramebufferSpec m_spec;
    uint32_t m_fbo;
    std::vector<std::unique_ptr<FramebufferAttachment>> m_attachments;
};

/**
 * @brief Framebuffer manager
 */
class FramebufferManager {
public:
    FramebufferManager();
    ~FramebufferManager();
    
    // Framebuffer creation
    uint32_t createFramebuffer(const std::string& name, const FramebufferSpec& spec);
    void destroyFramebuffer(uint32_t id);
    
    // Access
    Framebuffer* getFramebuffer(uint32_t id);
    const Framebuffer* getFramebuffer(uint32_t id) const;
    uint32_t findFramebuffer(const std::string& name) const;
    
    // Binding
    void bindFramebuffer(uint32_t id);
    void unbindFramebuffer();
    uint32_t getCurrentFramebuffer() const { return m_currentFBO; }
    
    // Common framebuffer configurations
    uint32_t createColorOnlyFBO(const std::string& name, uint32_t width, uint32_t height,
                               TextureFormat format = TextureFormat::RGBA8);
    
    uint32_t createColorDepthFBO(const std::string& name, uint32_t width, uint32_t height,
                                TextureFormat colorFormat = TextureFormat::RGBA8,
                                TextureFormat depthFormat = TextureFormat::Depth24);
    
    uint32_t createMultisampledFBO(const std::string& name, uint32_t width, uint32_t height,
                                  uint32_t samples = 4);
    
    uint32_t createGBufferFBO(const std::string& name, uint32_t width, uint32_t height);
    
    uint32_t createShadowMapFBO(const std::string& name, uint32_t resolution);
    
    // Resize all framebuffers (useful for window resize)
    void resizeAll(uint32_t width, uint32_t height);
    
    // Statistics
    struct Statistics {
        uint32_t framebufferCount;
        uint32_t totalMemoryUsage;
        uint32_t drawCallsToFBO;
        
        Statistics()
            : framebufferCount(0)
            , totalMemoryUsage(0)
            , drawCallsToFBO(0) {}
    };
    
    const Statistics& getStatistics() const { return m_stats; }
    void resetStatistics();
    
    // Debug
    void dumpFramebufferInfo() const;
    
private:
    std::unordered_map<uint32_t, std::unique_ptr<Framebuffer>> m_framebuffers;
    std::unordered_map<std::string, uint32_t> m_nameMap;
    uint32_t m_nextId;
    uint32_t m_currentFBO;
    Statistics m_stats;
};

/**
 * @brief Render target helper
 */
class RenderTarget {
public:
    RenderTarget(Framebuffer* framebuffer);
    ~RenderTarget();
    
    // Scoped binding - automatically unbinds on destruction
    void bind();
    void unbind();
    
    Framebuffer* getFramebuffer() { return m_framebuffer; }
    
private:
    Framebuffer* m_framebuffer;
    uint32_t m_previousFBO;
    bool m_bound;
};

} // namespace Graphics
} // namespace JJM

#endif // FRAMEBUFFER_MANAGER_H
