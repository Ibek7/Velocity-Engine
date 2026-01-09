#include "graphics/RenderGraph.h"
#include <algorithm>
#include <queue>
#include <sstream>
#include <GL/glew.h>

namespace JJM {
namespace Graphics {

RenderGraph::RenderGraph()
    : m_compiled(false)
{}

RenderGraph::~RenderGraph() {
    clear();
}

RenderPass& RenderGraph::addPass(const std::string& name) {
    auto pass = std::make_unique<RenderPass>(name);
    RenderPass* passPtr = pass.get();
    m_passes[name] = std::move(pass);
    m_compiled = false;
    return *passPtr;
}

void RenderGraph::declareResource(const ResourceDescriptor& descriptor) {
    m_resources[descriptor.name] = descriptor;
    m_compiled = false;
}

void RenderGraph::setExternalResource(const std::string& name, unsigned int handle) {
    PhysicalResource resource;
    resource.handle = handle;
    resource.allocated = true;
    resource.type = ResourceType::Texture;  // Assume texture for external resources
    m_physicalResources[name] = resource;
}

unsigned int RenderGraph::getPhysicalResource(const std::string& name) const {
    auto it = m_physicalResources.find(name);
    if (it != m_physicalResources.end()) {
        return it->second.handle;
    }
    
    // Check if resource is aliased
    auto aliasIt = m_resourceAliases.find(name);
    if (aliasIt != m_resourceAliases.end()) {
        return getPhysicalResource(aliasIt->second);
    }
    
    return 0;
}

void RenderGraph::compile() {
    // Clear previous compilation
    m_executionOrder.clear();
    
    // Build dependency graph and determine execution order
    buildDependencyGraph();
    
    // Cull unused passes
    cullPasses();
    
    // Allocate physical resources
    allocateResources();
    
    // Optimize memory usage
    optimizeMemory();
    
    m_compiled = true;
}

void RenderGraph::execute() {
    if (!m_compiled) {
        compile();
    }
    
    for (RenderPass* pass : m_executionOrder) {
        if (!pass->isCulled()) {
            pass->execute();
        }
    }
}

void RenderGraph::clear() {
    m_passes.clear();
    m_resources.clear();
    
    // Free allocated physical resources
    for (auto& [name, resource] : m_physicalResources) {
        if (resource.allocated && resource.handle != 0) {
            // Delete GPU resources based on type
            switch (resource.type) {
                case ResourceType::Texture:
                case ResourceType::RenderTarget:
                    glDeleteTextures(1, &resource.handle);
                    break;
                case ResourceType::Buffer:
                    glDeleteBuffers(1, &resource.handle);
                    break;
            }
        }
    }
    
    m_physicalResources.clear();
    m_resourceAliases.clear();
    m_executionOrder.clear();
    m_compiled = false;
}

std::string RenderGraph::exportDOT() const {
    std::ostringstream oss;
    
    oss << "digraph RenderGraph {\n";
    oss << "  rankdir=LR;\n";
    oss << "  node [shape=box];\n\n";
    
    // Add nodes for passes
    for (const auto& [name, pass] : m_passes) {
        std::string color = pass->isCulled() ? "gray" : "lightblue";
        oss << "  \"" << name << "\" [style=filled,fillcolor=" << color << "];\n";
    }
    
    // Add nodes for resources
    oss << "\n  node [shape=ellipse,style=filled,fillcolor=lightgreen];\n";
    for (const auto& [name, desc] : m_resources) {
        oss << "  \"" << name << "\";\n";
    }
    
    // Add edges
    oss << "\n";
    for (const auto& [passName, pass] : m_passes) {
        for (const auto& input : pass->getInputs()) {
            oss << "  \"" << input.resourceName << "\" -> \"" << passName << "\";\n";
        }
        for (const auto& output : pass->getOutputs()) {
            oss << "  \"" << passName << "\" -> \"" << output.resourceName << "\";\n";
        }
    }
    
    oss << "}\n";
    return oss.str();
}

RenderGraph::Stats RenderGraph::getStats() const {
    Stats stats = {};
    stats.totalPasses = static_cast<int>(m_passes.size());
    stats.totalResources = static_cast<int>(m_resources.size());
    
    for (const auto& [name, pass] : m_passes) {
        if (pass->isCulled()) {
            stats.culledPasses++;
        } else {
            stats.executedPasses++;
        }
    }
    
    for (const auto& [name, desc] : m_resources) {
        if (desc.isTransient) {
            stats.transientResources++;
        }
    }
    
    // Calculate memory usage (simplified)
    for (const auto& [name, desc] : m_resources) {
        if (desc.type == ResourceType::Texture) {
            // Estimate bytes per pixel based on format
            int bytesPerPixel = 4;  // RGBA8 default
            stats.memoryUsed += desc.width * desc.height * bytesPerPixel;
        }
    }
    
    return stats;
}

void RenderGraph::buildDependencyGraph() {
    // Perform topological sort to determine execution order
    if (!topologicalSort()) {
        // Cyclic dependency detected - error handling
        // For now, just use insertion order
        m_executionOrder.clear();
        for (auto& [name, pass] : m_passes) {
            m_executionOrder.push_back(pass.get());
        }
    }
}

bool RenderGraph::topologicalSort() {
    // Calculate in-degree for each pass
    std::unordered_map<RenderPass*, int> inDegree;
    std::unordered_map<RenderPass*, std::vector<RenderPass*>> adjacency;
    
    // Initialize
    for (auto& [name, pass] : m_passes) {
        inDegree[pass.get()] = 0;
    }
    
    // Build adjacency list and calculate in-degrees
    for (auto& [name, pass] : m_passes) {
        for (const auto& input : pass->getInputs()) {
            // Find all passes that write to this input
            auto writers = getWriters(input.resourceName);
            for (auto* writer : writers) {
                if (writer != pass.get()) {
                    adjacency[writer].push_back(pass.get());
                    inDegree[pass.get()]++;
                }
            }
        }
    }
    
    // Kahn's algorithm for topological sort
    std::queue<RenderPass*> queue;
    for (auto& [pass, degree] : inDegree) {
        if (degree == 0) {
            queue.push(pass);
        }
    }
    
    m_executionOrder.clear();
    int order = 0;
    
    while (!queue.empty()) {
        RenderPass* pass = queue.front();
        queue.pop();
        
        pass->setExecutionOrder(order++);
        m_executionOrder.push_back(pass);
        
        // Reduce in-degree for dependent passes
        for (RenderPass* dependent : adjacency[pass]) {
            inDegree[dependent]--;
            if (inDegree[dependent] == 0) {
                queue.push(dependent);
            }
        }
    }
    
    // Check if all passes were processed (no cycles)
    return m_executionOrder.size() == m_passes.size();
}

void RenderGraph::cullPasses() {
    // Mark all passes as culled initially
    for (auto& [name, pass] : m_passes) {
        pass->setCulled(true);
    }
    
    // Find output passes (passes whose outputs are not consumed or are external)
    std::set<RenderPass*> activePasses;
    
    for (auto& [name, pass] : m_passes) {
        for (const auto& output : pass->getOutputs()) {
            // Check if output is used by external systems or marked as important
            auto readers = getReaders(output.resourceName);
            if (readers.empty() || m_physicalResources.count(output.resourceName) > 0) {
                // This is an output pass
                activePasses.insert(pass.get());
            }
        }
    }
    
    // Recursively mark dependencies as active
    std::queue<RenderPass*> queue;
    for (RenderPass* pass : activePasses) {
        queue.push(pass);
    }
    
    while (!queue.empty()) {
        RenderPass* pass = queue.front();
        queue.pop();
        
        if (!pass->isCulled()) {
            continue;  // Already processed
        }
        
        pass->setCulled(false);
        
        // Add dependencies to queue
        for (const auto& input : pass->getInputs()) {
            auto writers = getWriters(input.resourceName);
            for (RenderPass* writer : writers) {
                if (writer->isCulled()) {
                    queue.push(writer);
                }
            }
        }
    }
}

void RenderGraph::allocateResources() {
    for (const auto& [name, desc] : m_resources) {
        // Skip if already allocated (external resource)
        if (m_physicalResources.count(name) > 0) {
            continue;
        }
        
        PhysicalResource physical;
        physical.type = desc.type;
        
        switch (desc.type) {
            case ResourceType::Texture:
            case ResourceType::RenderTarget: {
                unsigned int texture;
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexImage2D(GL_TEXTURE_2D, 0, desc.format, desc.width, desc.height,
                           0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                physical.handle = texture;
                break;
            }
            case ResourceType::Buffer: {
                unsigned int buffer;
                glGenBuffers(1, &buffer);
                physical.handle = buffer;
                break;
            }
        }
        
        physical.allocated = true;
        m_physicalResources[name] = physical;
    }
}

void RenderGraph::optimizeMemory() {
    // Implement resource aliasing for transient resources
    // Resources can be aliased if their lifetimes don't overlap
    
    // TODO: Implement lifetime analysis and aliasing optimization
    // For now, this is a placeholder
}

bool RenderGraph::hasDataDependency(const RenderPass* pass, const RenderPass* other) const {
    // Check if 'pass' depends on 'other' through data flow
    for (const auto& input : pass->getInputs()) {
        for (const auto& output : other->getOutputs()) {
            if (input.resourceName == output.resourceName) {
                return true;
            }
        }
    }
    return false;
}

std::vector<RenderPass*> RenderGraph::getWriters(const std::string& resource) {
    std::vector<RenderPass*> writers;
    for (auto& [name, pass] : m_passes) {
        for (const auto& output : pass->getOutputs()) {
            if (output.resourceName == resource) {
                writers.push_back(pass.get());
                break;
            }
        }
    }
    return writers;
}

std::vector<RenderPass*> RenderGraph::getReaders(const std::string& resource) {
    std::vector<RenderPass*> readers;
    for (auto& [name, pass] : m_passes) {
        for (const auto& input : pass->getInputs()) {
            if (input.resourceName == resource) {
                readers.push_back(pass.get());
                break;
            }
        }
    }
    return readers;
}

} // namespace Graphics
} // namespace JJM
