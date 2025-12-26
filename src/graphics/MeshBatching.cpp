#include "graphics/MeshBatching.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace Engine {

MeshBatchingSystem::MeshBatchingSystem()
    : m_nextMeshId(0)
{
}

MeshBatchingSystem::~MeshBatchingSystem() {
    shutdown();
}

void MeshBatchingSystem::initialize() {
    resetStats();
}

void MeshBatchingSystem::shutdown() {
    for (auto& pair : m_batches) {
        destroyBatch(pair.second.batched);
    }
    m_batches.clear();
    m_renderOrder.clear();
}

void MeshBatchingSystem::setSettings(const BatchSettings& settings) {
    m_settings = settings;
}

int MeshBatchingSystem::addMesh(const MeshData& mesh, BatchStrategy strategy) {
    int meshId = m_nextMeshId++;
    
    // Select strategy if automatic
    BatchStrategy finalStrategy = strategy;
    if (strategy == BatchStrategy::Automatic) {
        finalStrategy = selectStrategy(mesh);
    }
    
    // Generate batch key from material and texture
    std::string key = generateKey(mesh.materialId, mesh.texturePath);
    
    // Find or create batch group
    auto it = m_batches.find(key);
    if (it == m_batches.end()) {
        BatchGroup newGroup;
        newGroup.key = key;
        newGroup.strategy = finalStrategy;
        m_batches[key] = newGroup;
        it = m_batches.find(key);
    }
    
    // Add mesh to group
    it->second.meshes.push_back(mesh);
    it->second.isDirty = true;
    
    return meshId;
}

void MeshBatchingSystem::removeMesh(int meshId) {
    // In production, would track mesh ID to batch mapping
    // For now, mark all batches as dirty
    for (auto& pair : m_batches) {
        pair.second.isDirty = true;
    }
}

void MeshBatchingSystem::updateMesh(int meshId, const MeshData& mesh) {
    // Mark relevant batches as dirty
    std::string key = generateKey(mesh.materialId, mesh.texturePath);
    auto it = m_batches.find(key);
    if (it != m_batches.end()) {
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::clearMeshes() {
    shutdown();
    initialize();
}

void MeshBatchingSystem::addInstance(const std::string& meshKey, const InstanceData& instance) {
    auto it = m_batches.find(meshKey);
    if (it != m_batches.end()) {
        it->second.instances.push_back(instance);
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::updateInstance(const std::string& meshKey, int instanceIndex, const InstanceData& instance) {
    auto it = m_batches.find(meshKey);
    if (it != m_batches.end() && instanceIndex >= 0 && 
        instanceIndex < static_cast<int>(it->second.instances.size())) {
        it->second.instances[instanceIndex] = instance;
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::removeInstance(const std::string& meshKey, int instanceIndex) {
    auto it = m_batches.find(meshKey);
    if (it != m_batches.end() && instanceIndex >= 0 && 
        instanceIndex < static_cast<int>(it->second.instances.size())) {
        it->second.instances.erase(it->second.instances.begin() + instanceIndex);
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::clearInstances(const std::string& meshKey) {
    auto it = m_batches.find(meshKey);
    if (it != m_batches.end()) {
        it->second.instances.clear();
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::buildBatches() {
    for (auto& pair : m_batches) {
        BatchGroup& group = pair.second;
        
        // Skip if not dirty and not enough meshes
        if (!group.isDirty) {
            continue;
        }
        
        if (static_cast<int>(group.meshes.size()) < m_settings.minMeshesForBatching &&
            group.instances.empty()) {
            continue;
        }
        
        // Build batch based on strategy
        switch (group.strategy) {
            case BatchStrategy::Static:
                if (m_settings.enableStaticBatching) {
                    buildStaticBatch(group);
                }
                break;
            case BatchStrategy::Dynamic:
                if (m_settings.enableDynamicBatching) {
                    buildDynamicBatch(group);
                }
                break;
            case BatchStrategy::Instanced:
                if (m_settings.enableInstancing) {
                    buildInstancedBatch(group);
                }
                break;
            case BatchStrategy::Automatic:
                // Should have been resolved in addMesh
                break;
        }
        
        group.isDirty = false;
    }
    
    sortBatchesByMaterial();
    calculateStats();
}

void MeshBatchingSystem::rebuildDirtyBatches() {
    for (auto& pair : m_batches) {
        if (pair.second.isDirty) {
            buildBatches();
            break;
        }
    }
}

void MeshBatchingSystem::markDirty(const std::string& key) {
    auto it = m_batches.find(key);
    if (it != m_batches.end()) {
        it->second.isDirty = true;
    }
}

void MeshBatchingSystem::buildStaticBatch(BatchGroup& group) {
    if (group.meshes.empty()) {
        return;
    }
    
    // Destroy old batch
    destroyBatch(group.batched);
    
    // Combine meshes
    combineStaticMeshes(group.meshes, group.batched);
    
    group.batched.strategy = BatchStrategy::Static;
}

void MeshBatchingSystem::combineStaticMeshes(const std::vector<MeshData>& meshes, BatchedMesh& result) {
    std::vector<BatchVertex> combinedVertices;
    std::vector<unsigned int> combinedIndices;
    
    unsigned int vertexOffset = 0;
    
    for (const auto& mesh : meshes) {
        // Transform vertices by mesh transform
        for (const auto& vertex : mesh.vertices) {
            BatchVertex transformedVertex = vertex;
            
            // Apply transform matrix
            const float* m = mesh.transformMatrix;
            float x = vertex.x;
            float y = vertex.y;
            float z = vertex.z;
            
            transformedVertex.x = m[0]*x + m[4]*y + m[8]*z + m[12];
            transformedVertex.y = m[1]*x + m[5]*y + m[9]*z + m[13];
            transformedVertex.z = m[2]*x + m[6]*y + m[10]*z + m[14];
            
            // Transform normal
            float nx = vertex.nx;
            float ny = vertex.ny;
            float nz = vertex.nz;
            transformedVertex.nx = m[0]*nx + m[4]*ny + m[8]*nz;
            transformedVertex.ny = m[1]*nx + m[5]*ny + m[9]*nz;
            transformedVertex.nz = m[2]*nx + m[6]*ny + m[10]*nz;
            
            combinedVertices.push_back(transformedVertex);
        }
        
        // Add indices with offset
        for (unsigned int index : mesh.indices) {
            combinedIndices.push_back(index + vertexOffset);
        }
        
        vertexOffset += static_cast<unsigned int>(mesh.vertices.size());
    }
    
    // Create GPU buffers
    createVertexBuffer(result, combinedVertices);
    createIndexBuffer(result, combinedIndices);
    
    result.vertexCount = static_cast<int>(combinedVertices.size());
    result.indexCount = static_cast<int>(combinedIndices.size());
}

void MeshBatchingSystem::buildDynamicBatch(BatchGroup& group) {
    // Dynamic batching similar to static but without transform baking
    buildStaticBatch(group);
    group.batched.strategy = BatchStrategy::Dynamic;
}

bool MeshBatchingSystem::canBatchDynamic(const MeshData& a, const MeshData& b) const {
    // Check if meshes can be dynamically batched together
    if (a.materialId != b.materialId) return false;
    if (a.texturePath != b.texturePath) return false;
    if (a.vertices.size() + b.vertices.size() > static_cast<size_t>(m_settings.maxVerticesPerBatch)) return false;
    
    return true;
}

void MeshBatchingSystem::buildInstancedBatch(BatchGroup& group) {
    if (group.instances.empty() || group.meshes.empty()) {
        return;
    }
    
    // Use first mesh as template
    const MeshData& templateMesh = group.meshes[0];
    
    // Destroy old batch
    destroyBatch(group.batched);
    
    // Create buffers
    createVertexBuffer(group.batched, templateMesh.vertices);
    createIndexBuffer(group.batched, templateMesh.indices);
    createInstanceBuffer(group.batched, group.instances);
    
    group.batched.vertexCount = static_cast<int>(templateMesh.vertices.size());
    group.batched.indexCount = static_cast<int>(templateMesh.indices.size());
    group.batched.instanceCount = static_cast<int>(group.instances.size());
    group.batched.strategy = BatchStrategy::Instanced;
}

void MeshBatchingSystem::updateInstanceBuffer(BatchGroup& group) {
    if (group.batched.instanceBuffer == 0) {
        return;
    }
    
    // Update instance buffer data
    // glBindBuffer(GL_ARRAY_BUFFER, group.batched.instanceBuffer);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, group.instances.size() * sizeof(InstanceData), group.instances.data());
}

void MeshBatchingSystem::render() {
    for (const auto& key : m_renderOrder) {
        auto it = m_batches.find(key);
        if (it != m_batches.end()) {
            const BatchedMesh& batch = it->second.batched;
            
            if (batch.strategy == BatchStrategy::Instanced) {
                renderInstanced(batch);
            } else {
                renderBatch(batch);
            }
        }
    }
}

void MeshBatchingSystem::renderBatch(const BatchedMesh& batch) {
    if (batch.vertexBuffer == 0 || batch.indexBuffer == 0) {
        return;
    }
    
    // Bind buffers and draw
    // glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBuffer);
    // glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, 0);
}

void MeshBatchingSystem::renderInstanced(const BatchedMesh& batch) {
    if (batch.vertexBuffer == 0 || batch.indexBuffer == 0 || batch.instanceBuffer == 0) {
        return;
    }
    
    // Bind buffers and draw instanced
    // glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBuffer);
    // glDrawElementsInstanced(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, 0, batch.instanceCount);
}

const BatchedMesh* MeshBatchingSystem::getBatch(const std::string& key) const {
    auto it = m_batches.find(key);
    if (it != m_batches.end()) {
        return &it->second.batched;
    }
    return nullptr;
}

std::vector<std::string> MeshBatchingSystem::getBatchKeys() const {
    std::vector<std::string> keys;
    keys.reserve(m_batches.size());
    for (const auto& pair : m_batches) {
        keys.push_back(pair.first);
    }
    return keys;
}

void MeshBatchingSystem::optimizeBatches() {
    sortBatchesByMaterial();
    mergeSimilarBatches();
}

void MeshBatchingSystem::sortBatchesByMaterial() {
    m_renderOrder.clear();
    m_renderOrder.reserve(m_batches.size());
    
    for (const auto& pair : m_batches) {
        m_renderOrder.push_back(pair.first);
    }
    
    // Sort by material ID (extracted from key)
    std::sort(m_renderOrder.begin(), m_renderOrder.end());
}

void MeshBatchingSystem::mergeSimilarBatches() {
    // Find batches with same material that can be merged
    // Implementation would merge compatible batches to reduce draw calls
}

void MeshBatchingSystem::resetStats() {
    m_stats = BatchStats();
}

void MeshBatchingSystem::calculateStats() {
    m_stats.totalMeshes = 0;
    m_stats.batchedMeshes = 0;
    m_stats.instancedMeshes = 0;
    m_stats.drawCalls = 0;
    m_stats.totalVertices = 0;
    m_stats.totalIndices = 0;
    
    for (const auto& pair : m_batches) {
        const BatchGroup& group = pair.second;
        m_stats.totalMeshes += static_cast<int>(group.meshes.size());
        
        if (group.batched.strategy == BatchStrategy::Instanced) {
            m_stats.instancedMeshes += group.batched.instanceCount;
            m_stats.drawCalls += 1; // One draw call for all instances
        } else if (group.batched.vertexBuffer != 0) {
            m_stats.batchedMeshes += static_cast<int>(group.meshes.size());
            m_stats.drawCalls += 1;
        }
        
        m_stats.totalVertices += group.batched.vertexCount;
        m_stats.totalIndices += group.batched.indexCount;
    }
    
    // Calculate draw calls saved
    m_stats.drawCallsSaved = m_stats.totalMeshes - m_stats.drawCalls;
}

std::string MeshBatchingSystem::generateKey(unsigned int materialId, const std::string& texture) const {
    return std::to_string(materialId) + "_" + texture;
}

BatchStrategy MeshBatchingSystem::selectStrategy(const MeshData& mesh) const {
    // Heuristic to select best strategy
    // Check if mesh has identity transform (good for instancing)
    const float* m = mesh.transformMatrix;
    bool isIdentity = (m[0] == 1.0f && m[5] == 1.0f && m[10] == 1.0f && m[15] == 1.0f &&
                       m[12] == 0.0f && m[13] == 0.0f && m[14] == 0.0f);
    
    if (isIdentity && m_settings.enableInstancing) {
        return BatchStrategy::Instanced;
    }
    
    // Small meshes are good for static batching
    if (mesh.vertices.size() < 1000 && m_settings.enableStaticBatching) {
        return BatchStrategy::Static;
    }
    
    // Default to dynamic
    return BatchStrategy::Dynamic;
}

void MeshBatchingSystem::createVertexBuffer(BatchedMesh& batch, const std::vector<BatchVertex>& vertices) {
    // glGenBuffers(1, &batch.vertexBuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, batch.vertexBuffer);
    // glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(BatchVertex), vertices.data(), GL_STATIC_DRAW);
}

void MeshBatchingSystem::createIndexBuffer(BatchedMesh& batch, const std::vector<unsigned int>& indices) {
    // glGenBuffers(1, &batch.indexBuffer);
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.indexBuffer);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

void MeshBatchingSystem::createInstanceBuffer(BatchedMesh& batch, const std::vector<InstanceData>& instances) {
    // glGenBuffers(1, &batch.instanceBuffer);
    // glBindBuffer(GL_ARRAY_BUFFER, batch.instanceBuffer);
    // glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(InstanceData), instances.data(), GL_DYNAMIC_DRAW);
}

void MeshBatchingSystem::destroyBatch(BatchedMesh& batch) {
    // glDeleteBuffers(1, &batch.vertexBuffer);
    // glDeleteBuffers(1, &batch.indexBuffer);
    // glDeleteBuffers(1, &batch.instanceBuffer);
    
    batch.vertexBuffer = 0;
    batch.indexBuffer = 0;
    batch.instanceBuffer = 0;
}

// BatchingManager implementation
BatchingManager& BatchingManager::getInstance() {
    static BatchingManager instance;
    return instance;
}

void BatchingManager::initialize() {
    m_batchingSystem.initialize();
}

void BatchingManager::shutdown() {
    m_batchingSystem.shutdown();
}

} // namespace Engine
