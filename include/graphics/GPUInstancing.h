#ifndef GPU_INSTANCING_H
#define GPU_INSTANCING_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>

namespace JJM {
namespace Graphics {

/**
 * @brief Instance data for a single object
 */
struct InstanceData {
    float transform[16];     // 4x4 transformation matrix
    float color[4];          // RGBA color
    float customData[4];     // Custom per-instance data
    uint32_t textureIndex;   // Texture array index
    uint32_t materialId;     // Material identifier
    
    InstanceData();
};

/**
 * @brief Configuration for instancing system
 */
struct InstancingConfig {
    uint32_t maxInstancesPerBatch;
    uint32_t bufferUpdateFrequency;  // Frames between buffer updates
    bool dynamicBuffers;             // Use dynamic vs static buffers
    bool frustumCulling;             // Enable per-instance culling
    bool distanceCulling;            // Cull distant instances
    float maxRenderDistance;
    
    InstancingConfig()
        : maxInstancesPerBatch(1000)
        , bufferUpdateFrequency(1)
        , dynamicBuffers(true)
        , frustumCulling(true)
        , distanceCulling(true)
        , maxRenderDistance(1000.0f) {}
};

/**
 * @brief Statistics for instanced rendering
 */
struct InstancingStatistics {
    uint32_t totalInstances;
    uint32_t instancesRendered;
    uint32_t instancesCulled;
    uint32_t drawCalls;
    uint32_t batchCount;
    float cullingTime;
    float bufferUpdateTime;
    
    InstancingStatistics()
        : totalInstances(0)
        , instancesRendered(0)
        , instancesCulled(0)
        , drawCalls(0)
        , batchCount(0)
        , cullingTime(0.0f)
        , bufferUpdateTime(0.0f) {}
};

/**
 * @brief Batch of instances for a specific mesh
 */
class InstanceBatch {
public:
    InstanceBatch(uint32_t meshId, uint32_t maxInstances);
    ~InstanceBatch();
    
    uint32_t getMeshId() const { return m_meshId; }
    uint32_t getInstanceCount() const { return m_instanceCount; }
    uint32_t getMaxInstances() const { return m_maxInstances; }
    
    // Instance management
    uint32_t addInstance(const InstanceData& data);
    bool updateInstance(uint32_t index, const InstanceData& data);
    void removeInstance(uint32_t index);
    void clearInstances();
    
    // Buffer management
    void updateBuffer();
    uint32_t getInstanceBuffer() const { return m_instanceBuffer; }
    bool needsUpdate() const { return m_dirty; }
    void markDirty() { m_dirty = true; }
    
    // Culling
    void cullInstances(const float* frustumPlanes, const float* cameraPosition, float maxDistance);
    const std::vector<uint32_t>& getVisibleInstances() const { return m_visibleInstances; }
    
    // Data access
    const InstanceData* getInstanceData(uint32_t index) const;
    const std::vector<InstanceData>& getAllInstanceData() const { return m_instances; }
    
private:
    uint32_t m_meshId;
    uint32_t m_maxInstances;
    uint32_t m_instanceCount;
    uint32_t m_instanceBuffer;
    bool m_dirty;
    
    std::vector<InstanceData> m_instances;
    std::vector<uint32_t> m_freeIndices;
    std::vector<uint32_t> m_visibleInstances;
};

/**
 * @brief Main GPU instancing system
 */
class GPUInstancingSystem {
public:
    GPUInstancingSystem();
    ~GPUInstancingSystem();
    
    // Configuration
    void configure(const InstancingConfig& config);
    const InstancingConfig& getConfig() const { return m_config; }
    
    // Batch management
    uint32_t createBatch(uint32_t meshId);
    void destroyBatch(uint32_t batchId);
    InstanceBatch* getBatch(uint32_t batchId);
    const InstanceBatch* getBatch(uint32_t batchId) const;
    
    // Find batch by mesh ID
    InstanceBatch* findBatchForMesh(uint32_t meshId);
    
    // Instance management
    uint32_t addInstance(uint32_t meshId, const InstanceData& data);
    bool updateInstance(uint32_t batchId, uint32_t instanceId, const InstanceData& data);
    void removeInstance(uint32_t batchId, uint32_t instanceId);
    
    // Rendering
    void update(float deltaTime);
    void updateCulling(const float* frustumPlanes, const float* cameraPosition);
    void renderBatches();
    void renderBatch(uint32_t batchId);
    
    // Statistics
    const InstancingStatistics& getStatistics() const { return m_stats; }
    void resetStatistics();
    
    // Debug
    void enableDebugVisualization(bool enable) { m_debugVisualization = enable; }
    bool isDebugVisualizationEnabled() const { return m_debugVisualization; }
    void dumpBatchInfo() const;
    
private:
    void updateBuffers();
    void performCulling(const float* frustumPlanes, const float* cameraPosition);
    bool isInstanceVisible(const InstanceData& instance, 
                          const float* frustumPlanes,
                          const float* cameraPosition) const;
    
    InstancingConfig m_config;
    InstancingStatistics m_stats;
    
    std::unordered_map<uint32_t, std::unique_ptr<InstanceBatch>> m_batches;
    uint32_t m_nextBatchId;
    
    uint32_t m_frameCounter;
    bool m_debugVisualization;
};

/**
 * @brief Helper for building instance data
 */
class InstanceDataBuilder {
public:
    InstanceDataBuilder();
    
    InstanceDataBuilder& setTransform(const float* matrix);
    InstanceDataBuilder& setPosition(float x, float y, float z);
    InstanceDataBuilder& setRotation(float x, float y, float z, float w);
    InstanceDataBuilder& setScale(float x, float y, float z);
    InstanceDataBuilder& setColor(float r, float g, float b, float a);
    InstanceDataBuilder& setCustomData(const float* data);
    InstanceDataBuilder& setTextureIndex(uint32_t index);
    InstanceDataBuilder& setMaterialId(uint32_t id);
    
    InstanceData build() const;
    
private:
    InstanceData m_data;
};

} // namespace Graphics
} // namespace JJM

#endif // GPU_INSTANCING_H
