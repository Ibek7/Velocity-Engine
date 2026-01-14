#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace Graphics {

/**
 * @brief Render pass system for organizing rendering pipeline
 */

class Framebuffer;
class Shader;

/**
 * @brief Clear flags for render pass
 */
enum ClearFlags {
    ClearNone = 0,
    ClearColor = 1 << 0,
    ClearDepth = 1 << 1,
    ClearStencil = 1 << 2,
    ClearAll = ClearColor | ClearDepth | ClearStencil
};

/**
 * @brief Render pass configuration
 */
struct RenderPassConfig {
    std::string name;
    uint32_t framebuffer;
    uint32_t clearFlags;
    float clearColor[4];
    float clearDepth;
    int32_t clearStencil;
    bool enabled;
    
    RenderPassConfig()
        : framebuffer(0)
        , clearFlags(ClearAll)
        , clearDepth(1.0f)
        , clearStencil(0)
        , enabled(true) {
        clearColor[0] = 0.0f;
        clearColor[1] = 0.0f;
        clearColor[2] = 0.0f;
        clearColor[3] = 1.0f;
    }
};

/**
 * @brief Base render pass
 */
class RenderPass {
public:
    RenderPass(const std::string& name);
    virtual ~RenderPass();
    
    const std::string& getName() const { return m_name; }
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
    void setFramebuffer(uint32_t fbo) { m_framebuffer = fbo; }
    uint32_t getFramebuffer() const { return m_framebuffer; }
    
    // Lifecycle
    virtual void setup() = 0;
    virtual void execute() = 0;
    virtual void cleanup() = 0;
    
protected:
    std::string m_name;
    bool m_enabled;
    uint32_t m_framebuffer;
};

/**
 * @brief Render pass manager
 */
class RenderPassManager {
public:
    RenderPassManager();
    ~RenderPassManager();
    
    // Pass management
    void addPass(std::unique_ptr<RenderPass> pass);
    void removePass(const std::string& name);
    RenderPass* getPass(const std::string& name);
    
    // Execution
    void executeAll();
    void executePass(const std::string& name);
    
    // Pass ordering
    void setPassOrder(const std::vector<std::string>& order);
    
    // Statistics
    uint32_t getPassCount() const { return static_cast<uint32_t>(m_passes.size()); }
    float getTotalExecutionTime() const { return m_totalTime; }
    
private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    std::vector<std::string> m_passOrder;
    float m_totalTime;
};

/**
 * @brief Lambda-based render pass for quick custom passes
 */
class LambdaRenderPass : public RenderPass {
public:
    using ExecuteFunc = std::function<void()>;
    
    LambdaRenderPass(const std::string& name, ExecuteFunc func);
    
    void setup() override {}
    void execute() override;
    void cleanup() override {}
    
private:
    ExecuteFunc m_executeFunc;
};

} // namespace Graphics
} // namespace JJM

#endif // RENDER_PASS_H
