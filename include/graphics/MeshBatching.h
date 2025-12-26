#pragma once

#include <vector>
#include <unordered_map>
#include <string>

namespace Engine {

/**
 * @brief Batching strategy types
 */
enum class BatchStrategy {
    Static,         // Batch static geometry (never changes)
    Dynamic,        // Batch dynamic objects (can move/change)
    Instanced,      // GPU instancing for identical meshes
    Automatic       // Automatically choose best strategy
};

/**
 * @brief Vertex data for batching
 */
struct BatchVertex {
    float x, y, z;          // Position
    float nx, ny, nz;       // Normal
    float u, v;             // Texture coordinates
    float tx, ty, tz;       // Tangent
    float r, g, b, a;       // Vertex color
};

/**
 * @brief Mesh data for batching
 */
struct MeshData {
    std::vector<BatchVertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int materialId;
    std::string texturePath;
    
    // Transform (for static batching)
    float transformMatrix[16];
};

/**
 * @brief Instance data for instanced rendering
 */
struct InstanceData {
    float transformMatrix[16];
    float colorR, colorG, colorB, colorA;
    int entityId;
};

/**
 * @brief Batched mesh result
 */
struct BatchedMesh {
    unsigned int vertexBuffer;
    unsigned int indexBuffer;
    unsigned int instanceBuffer;    // For instanced rendering
    
    int vertexCount;
    int indexCount;
    int instanceCount;
    
    unsigned int materialId;
    std::string texturePath;
    
    BatchStrategy strategy;
    bool needsRebuild;
    
    BatchedMesh()
        : vertexBuffer(0)
        , indexBuffer(0)
        , instanceBuffer(0)
        , vertexCount(0)
        , indexCount(0)
        , instanceCount(0)
        , materialId(0)
        , strategy(BatchStrategy::Static)
        , needsRebuild(false)
    {}
};

/**
 * @brief Batch group (meshes with same material/texture)
 */
struct BatchGroup {
    std::string key;                    // Material/texture key
    std::vector<MeshData> meshes;       // Source meshes
    std::vector<InstanceData> instances; // For instanced rendering
    BatchedMesh batched;                // Result
    BatchStrategy strategy;
    bool isDirty;
    
    BatchGroup()
        : strategy(BatchStrategy::Static)
        , isDirty(true)
    {}
};

/**
 * @brief Batching settings
 */
struct BatchSettings {
    bool enableStaticBatching;
    bool enableDynamicBatching;
    bool enableInstancing;
    int maxVerticesPerBatch;        // Limit batch size
    int maxInstancesPerBatch;
    int minMeshesForBatching;       // Don't batch if less than this
    float dynamicBatchDistance;     // Max distance for dynamic batching
    
    BatchSettings()
        : enableStaticBatching(true)
        , enableDynamicBatching(true)
        , enableInstancing(true)
        , maxVerticesPerBatch(65536)
        , maxInstancesPerBatch(1024)
        , minMeshesForBatching(2)
        , dynamicBatchDistance(50.0f)
    {}
};

/**
 * @brief Batching statistics
 */
struct BatchStats {
    int totalMeshes;
    int batchedMeshes;
    int instancedMeshes;
    int drawCalls;              // After batching
    int drawCallsSaved;         // Draw calls eliminated
    int totalVertices;
    int totalIndices;
    float batchBuildTime;       // Time to build batches (ms)
    
    BatchStats()
        : totalMeshes(0)
        , batchedMeshes(0)
        , instancedMeshes(0)
        , drawCalls(0)
        , drawCallsSaved(0)
        , totalVertices(0)
        , totalIndices(0)
        , batchBuildTime(0.0f)
    {}
};

/**
 * @brief Mesh batching system
 */
class MeshBatchingSystem {
public:
    MeshBatchingSystem();
    ~MeshBatchingSystem();
    
    // Initialization
    void initialize();
    void shutdown();
    
    // Configuration
    void setSettings(const BatchSettings& settings);
    const BatchSettings& getSettings() const { return m_settings; }
    
    // Mesh management
    int addMesh(const MeshData& mesh, BatchStrategy strategy = BatchStrategy::Automatic);
    void removeMesh(int meshId);
    void updateMesh(int meshId, const MeshData& mesh);
    void clearMeshes();
    
    // Instance management (for instanced rendering)
    void addInstance(const std::string& meshKey, const InstanceData& instance);
    void updateInstance(const std::string& meshKey, int instanceIndex, const InstanceData& instance);
    void removeInstance(const std::string& meshKey, int instanceIndex);
    void clearInstances(const std::string& meshKey);
    
    // Batching
    void buildBatches();
    void rebuildDirtyBatches();
    void markDirty(const std::string& key);
    
    // Static batching
    void buildStaticBatch(BatchGroup& group);
    void combineStaticMeshes(const std::vector<MeshData>& meshes, BatchedMesh& result);
    
    // Dynamic batching
    void buildDynamicBatch(BatchGroup& group);
    bool canBatchDynamic(const MeshData& a, const MeshData& b) const;
    
    // Instanced rendering
    void buildInstancedBatch(BatchGroup& group);
    void updateInstanceBuffer(BatchGroup& group);
    
    // Rendering
    void render();
    void renderBatch(const BatchedMesh& batch);
    void renderInstanced(const BatchedMesh& batch);
    
    // Batch queries
    int getBatchCount() const { return static_cast<int>(m_batches.size()); }
    const BatchedMesh* getBatch(const std::string& key) const;
    std::vector<std::string> getBatchKeys() const;
    
    // Optimization
    void optimizeBatches();
    void sortBatchesByMaterial();
    void mergeSimilarBatches();
    
    // Statistics
    const BatchStats& getStats() const { return m_stats; }
    void resetStats();
    void calculateStats();
    
private:
    std::string generateKey(unsigned int materialId, const std::string& texture) const;
    BatchStrategy selectStrategy(const MeshData& mesh) const;
    void createVertexBuffer(BatchedMesh& batch, const std::vector<BatchVertex>& vertices);
    void createIndexBuffer(BatchedMesh& batch, const std::vector<unsigned int>& indices);
    void createInstanceBuffer(BatchedMesh& batch, const std::vector<InstanceData>& instances);
    void destroyBatch(BatchedMesh& batch);
    
    BatchSettings m_settings;
    std::unordered_map<std::string, BatchGroup> m_batches;
    std::vector<std::string> m_renderOrder; // Sorted batch keys
    int m_nextMeshId;
    BatchStats m_stats;
};

/**
 * @brief Global mesh batching system
 */
class BatchingManager {
public:
    static BatchingManager& getInstance();
    
    void initialize();
    void shutdown();
    
    MeshBatchingSystem& getBatchingSystem() { return m_batchingSystem; }
    
    // Convenience methods
    int addMesh(const MeshData& mesh, BatchStrategy strategy = BatchStrategy::Automatic) {
        return m_batchingSystem.addMesh(mesh, strategy);
    }
    void buildBatches() { m_batchingSystem.buildBatches(); }
    void render() { m_batchingSystem.render(); }
    
    // Enable/disable
    void enable(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    BatchingManager() : m_enabled(true) {}
    BatchingManager(const BatchingManager&) = delete;
    BatchingManager& operator=(const BatchingManager&) = delete;
    
    MeshBatchingSystem m_batchingSystem;
    bool m_enabled;
};

} // namespace Engine
