#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <set>

namespace JJM {
namespace Graphics {

/**
 * @brief Render resource type
 */
enum class ResourceType {
    Texture,
    Buffer,
    RenderTarget
};

/**
 * @brief Resource access type
 */
enum class ResourceAccess {
    Read,
    Write,
    ReadWrite
};

/**
 * @brief Render graph resource descriptor
 */
struct ResourceDescriptor {
    std::string name;
    ResourceType type;
    int width;
    int height;
    unsigned int format;
    bool isTransient;  // Can be aliased/reused
    
    ResourceDescriptor()
        : type(ResourceType::Texture)
        , width(0)
        , height(0)
        , format(0)
        , isTransient(true)
    {}
};

/**
 * @brief Resource reference in render pass
 */
struct ResourceRef {
    std::string resourceName;
    ResourceAccess access;
    
    ResourceRef(const std::string& name, ResourceAccess acc)
        : resourceName(name)
        , access(acc)
    {}
};

/**
 * @brief Render pass in the graph
 */
class RenderPass {
public:
    using ExecuteCallback = std::function<void()>;
    
private:
    std::string m_name;
    std::vector<ResourceRef> m_inputs;
    std::vector<ResourceRef> m_outputs;
    ExecuteCallback m_executeFunc;
    bool m_culled;
    int m_executionOrder;
    
public:
    RenderPass(const std::string& name)
        : m_name(name)
        , m_culled(false)
        , m_executionOrder(-1)
    {}
    
    void addInput(const std::string& resource, ResourceAccess access = ResourceAccess::Read) {
        m_inputs.emplace_back(resource, access);
    }
    
    void addOutput(const std::string& resource, ResourceAccess access = ResourceAccess::Write) {
        m_outputs.emplace_back(resource, access);
    }
    
    void setExecuteCallback(ExecuteCallback callback) {
        m_executeFunc = std::move(callback);
    }
    
    void execute() {
        if (m_executeFunc && !m_culled) {
            m_executeFunc();
        }
    }
    
    const std::string& getName() const { return m_name; }
    const std::vector<ResourceRef>& getInputs() const { return m_inputs; }
    const std::vector<ResourceRef>& getOutputs() const { return m_outputs; }
    bool isCulled() const { return m_culled; }
    void setCulled(bool culled) { m_culled = culled; }
    int getExecutionOrder() const { return m_executionOrder; }
    void setExecutionOrder(int order) { m_executionOrder = order; }
};

/**
 * @brief Physical resource handle
 */
struct PhysicalResource {
    unsigned int handle;  // GPU resource handle (texture, buffer, etc.)
    ResourceType type;
    bool allocated;
    
    PhysicalResource()
        : handle(0)
        , type(ResourceType::Texture)
        , allocated(false)
    {}
};

/**
 * @brief Render graph for automatic dependency management
 * 
 * Manages render passes and their dependencies, automatically
 * determining execution order, resource allocation, and synchronization.
 */
class RenderGraph {
private:
    std::unordered_map<std::string, std::unique_ptr<RenderPass>> m_passes;
    std::unordered_map<std::string, ResourceDescriptor> m_resources;
    std::unordered_map<std::string, PhysicalResource> m_physicalResources;
    std::vector<RenderPass*> m_executionOrder;
    bool m_compiled;
    
    // Resource aliasing for memory optimization
    std::unordered_map<std::string, std::string> m_resourceAliases;
    
public:
    RenderGraph();
    ~RenderGraph();
    
    /**
     * @brief Add a render pass to the graph
     * @param name Unique pass name
     * @return Reference to created pass for configuration
     */
    RenderPass& addPass(const std::string& name);
    
    /**
     * @brief Declare a resource used in the graph
     * @param descriptor Resource descriptor
     */
    void declareResource(const ResourceDescriptor& descriptor);
    
    /**
     * @brief Set external resource (imported from outside the graph)
     * @param name Resource name
     * @param handle Physical resource handle
     */
    void setExternalResource(const std::string& name, unsigned int handle);
    
    /**
     * @brief Get physical resource handle
     * @param name Resource name
     * @return Physical GPU resource handle
     */
    unsigned int getPhysicalResource(const std::string& name) const;
    
    /**
     * @brief Compile the render graph
     * 
     * Performs dependency analysis, determines execution order,
     * culls unused passes, and allocates resources.
     */
    void compile();
    
    /**
     * @brief Execute the compiled render graph
     */
    void execute();
    
    /**
     * @brief Clear all passes and resources
     */
    void clear();
    
    /**
     * @brief Check if graph is compiled
     */
    bool isCompiled() const { return m_compiled; }
    
    /**
     * @brief Export graph visualization in DOT format
     * @return DOT graph string for visualization
     */
    std::string exportDOT() const;
    
    /**
     * @brief Get execution statistics
     */
    struct Stats {
        int totalPasses;
        int executedPasses;
        int culledPasses;
        int totalResources;
        int transientResources;
        size_t memoryUsed;
    };
    
    Stats getStats() const;
    
private:
    /**
     * @brief Build dependency graph and determine execution order
     */
    void buildDependencyGraph();
    
    /**
     * @brief Perform topological sort for execution order
     */
    bool topologicalSort();
    
    /**
     * @brief Cull unused render passes
     */
    void cullPasses();
    
    /**
     * @brief Allocate physical resources
     */
    void allocateResources();
    
    /**
     * @brief Optimize resource memory by aliasing transient resources
     */
    void optimizeMemory();
    
    /**
     * @brief Check if pass depends on another pass
     */
    bool hasDataDependency(const RenderPass* pass, const RenderPass* other) const;
    
    /**
     * @brief Get all passes that write to a resource
     */
    std::vector<RenderPass*> getWriters(const std::string& resource);
    
    /**
     * @brief Get all passes that read from a resource
     */
    std::vector<RenderPass*> getReaders(const std::string& resource);
};

/**
 * @brief Render graph builder helper
 */
class RenderGraphBuilder {
private:
    RenderGraph& m_graph;
    RenderPass* m_currentPass;
    
public:
    explicit RenderGraphBuilder(RenderGraph& graph)
        : m_graph(graph)
        , m_currentPass(nullptr)
    {}
    
    RenderGraphBuilder& addPass(const std::string& name) {
        m_currentPass = &m_graph.addPass(name);
        return *this;
    }
    
    RenderGraphBuilder& read(const std::string& resource) {
        if (m_currentPass) {
            m_currentPass->addInput(resource, ResourceAccess::Read);
        }
        return *this;
    }
    
    RenderGraphBuilder& write(const std::string& resource) {
        if (m_currentPass) {
            m_currentPass->addOutput(resource, ResourceAccess::Write);
        }
        return *this;
    }
    
    RenderGraphBuilder& execute(RenderPass::ExecuteCallback callback) {
        if (m_currentPass) {
            m_currentPass->setExecuteCallback(std::move(callback));
        }
        return *this;
    }
    
    RenderGraph& build() {
        return m_graph;
    }
};

} // namespace Graphics
} // namespace JJM

#endif // RENDER_GRAPH_H
