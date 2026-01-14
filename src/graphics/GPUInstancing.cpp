#include "graphics/GPUInstancing.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Graphics {

// InstanceData Implementation
InstanceData::InstanceData() 
    : textureIndex(0)
    , materialId(0) {
    // Identity matrix
    for (int i = 0; i < 16; ++i) {
        transform[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
    
    // White color
    color[0] = color[1] = color[2] = color[3] = 1.0f;
    
    // Zero custom data
    customData[0] = customData[1] = customData[2] = customData[3] = 0.0f;
}

// InstanceBatch Implementation
InstanceBatch::InstanceBatch(uint32_t meshId, uint32_t maxInstances)
    : m_meshId(meshId)
    , m_maxInstances(maxInstances)
    , m_instanceCount(0)
    , m_instanceBuffer(0)
    , m_dirty(false) {
    m_instances.reserve(maxInstances);
    m_visibleInstances.reserve(maxInstances);
}

InstanceBatch::~InstanceBatch() {
    // TODO: Release GPU buffer
    if (m_instanceBuffer != 0) {
        // glDeleteBuffers(1, &m_instanceBuffer);
    }
}

uint32_t InstanceBatch::addInstance(const InstanceData& data) {
    uint32_t index;
    
    if (!m_freeIndices.empty()) {
        index = m_freeIndices.back();
        m_freeIndices.pop_back();
        m_instances[index] = data;
    } else {
        if (m_instanceCount >= m_maxInstances) {
            return UINT32_MAX;
        }
        index = m_instanceCount;
        m_instances.push_back(data);
    }
    
    m_instanceCount++;
    m_dirty = true;
    return index;
}

bool InstanceBatch::updateInstance(uint32_t index, const InstanceData& data) {
    if (index >= m_instances.size()) {
        return false;
    }
    
    m_instances[index] = data;
    m_dirty = true;
    return true;
}

void InstanceBatch::removeInstance(uint32_t index) {
    if (index >= m_instances.size()) {
        return;
    }
    
    m_freeIndices.push_back(index);
    m_instanceCount--;
    m_dirty = true;
}

void InstanceBatch::clearInstances() {
    m_instances.clear();
    m_freeIndices.clear();
    m_visibleInstances.clear();
    m_instanceCount = 0;
    m_dirty = true;
}

void InstanceBatch::updateBuffer() {
    if (!m_dirty) {
        return;
    }
    
    // TODO: Update GPU buffer with instance data
    // glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
    // glBufferData(GL_ARRAY_BUFFER, m_instances.size() * sizeof(InstanceData), 
    //              m_instances.data(), GL_DYNAMIC_DRAW);
    
    m_dirty = false;
}

void InstanceBatch::cullInstances(const float* frustumPlanes, const float* cameraPosition, float maxDistance) {
    m_visibleInstances.clear();
    
    for (uint32_t i = 0; i < m_instances.size(); ++i) {
        if (std::find(m_freeIndices.begin(), m_freeIndices.end(), i) != m_freeIndices.end()) {
            continue; // Skip free slots
        }
        
        const InstanceData& instance = m_instances[i];
        
        // Extract position from transform matrix
        float posX = instance.transform[12];
        float posY = instance.transform[13];
        float posZ = instance.transform[14];
        
        // Distance culling
        if (cameraPosition != nullptr && maxDistance > 0.0f) {
            float dx = posX - cameraPosition[0];
            float dy = posY - cameraPosition[1];
            float dz = posZ - cameraPosition[2];
            float distSq = dx * dx + dy * dy + dz * dz;
            
            if (distSq > maxDistance * maxDistance) {
                continue;
            }
        }
        
        // Frustum culling (simplified - assumes point check)
        if (frustumPlanes != nullptr) {
            bool visible = true;
            for (int p = 0; p < 6; ++p) {
                float distance = frustumPlanes[p * 4 + 0] * posX +
                               frustumPlanes[p * 4 + 1] * posY +
                               frustumPlanes[p * 4 + 2] * posZ +
                               frustumPlanes[p * 4 + 3];
                if (distance < 0.0f) {
                    visible = false;
                    break;
                }
            }
            if (!visible) {
                continue;
            }
        }
        
        m_visibleInstances.push_back(i);
    }
}

const InstanceData* InstanceBatch::getInstanceData(uint32_t index) const {
    if (index >= m_instances.size()) {
        return nullptr;
    }
    return &m_instances[index];
}

// GPUInstancingSystem Implementation
GPUInstancingSystem::GPUInstancingSystem()
    : m_nextBatchId(1)
    , m_frameCounter(0)
    , m_debugVisualization(false) {
}

GPUInstancingSystem::~GPUInstancingSystem() {
    m_batches.clear();
}

void GPUInstancingSystem::configure(const InstancingConfig& config) {
    m_config = config;
}

uint32_t GPUInstancingSystem::createBatch(uint32_t meshId) {
    uint32_t batchId = m_nextBatchId++;
    auto batch = std::make_unique<InstanceBatch>(meshId, m_config.maxInstancesPerBatch);
    m_batches[batchId] = std::move(batch);
    return batchId;
}

void GPUInstancingSystem::destroyBatch(uint32_t batchId) {
    m_batches.erase(batchId);
}

InstanceBatch* GPUInstancingSystem::getBatch(uint32_t batchId) {
    auto it = m_batches.find(batchId);
    return (it != m_batches.end()) ? it->second.get() : nullptr;
}

const InstanceBatch* GPUInstancingSystem::getBatch(uint32_t batchId) const {
    auto it = m_batches.find(batchId);
    return (it != m_batches.end()) ? it->second.get() : nullptr;
}

InstanceBatch* GPUInstancingSystem::findBatchForMesh(uint32_t meshId) {
    for (auto& pair : m_batches) {
        if (pair.second->getMeshId() == meshId) {
            return pair.second.get();
        }
    }
    return nullptr;
}

uint32_t GPUInstancingSystem::addInstance(uint32_t meshId, const InstanceData& data) {
    InstanceBatch* batch = findBatchForMesh(meshId);
    if (!batch) {
        uint32_t batchId = createBatch(meshId);
        batch = getBatch(batchId);
    }
    
    if (batch) {
        m_stats.totalInstances++;
        return batch->addInstance(data);
    }
    
    return UINT32_MAX;
}

bool GPUInstancingSystem::updateInstance(uint32_t batchId, uint32_t instanceId, const InstanceData& data) {
    InstanceBatch* batch = getBatch(batchId);
    return batch ? batch->updateInstance(instanceId, data) : false;
}

void GPUInstancingSystem::removeInstance(uint32_t batchId, uint32_t instanceId) {
    InstanceBatch* batch = getBatch(batchId);
    if (batch) {
        batch->removeInstance(instanceId);
        m_stats.totalInstances--;
    }
}

void GPUInstancingSystem::update(float deltaTime) {
    m_frameCounter++;
    
    // Update buffers periodically
    if (m_frameCounter % m_config.bufferUpdateFrequency == 0) {
        updateBuffers();
    }
    
    resetStatistics();
}

void GPUInstancingSystem::updateCulling(const float* frustumPlanes, const float* cameraPosition) {
    if (!m_config.frustumCulling && !m_config.distanceCulling) {
        return;
    }
    
    float maxDist = m_config.distanceCulling ? m_config.maxRenderDistance : 0.0f;
    const float* frustum = m_config.frustumCulling ? frustumPlanes : nullptr;
    
    for (auto& pair : m_batches) {
        pair.second->cullInstances(frustum, cameraPosition, maxDist);
    }
}

void GPUInstancingSystem::renderBatches() {
    m_stats.drawCalls = 0;
    m_stats.instancesRendered = 0;
    m_stats.batchCount = static_cast<uint32_t>(m_batches.size());
    
    for (auto& pair : m_batches) {
        InstanceBatch* batch = pair.second.get();
        
        uint32_t instanceCount = m_config.frustumCulling || m_config.distanceCulling
            ? static_cast<uint32_t>(batch->getVisibleInstances().size())
            : batch->getInstanceCount();
        
        if (instanceCount > 0) {
            // TODO: Actual draw call
            // glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0, instanceCount);
            
            m_stats.drawCalls++;
            m_stats.instancesRendered += instanceCount;
        }
        
        m_stats.instancesCulled += (batch->getInstanceCount() - instanceCount);
    }
}

void GPUInstancingSystem::renderBatch(uint32_t batchId) {
    InstanceBatch* batch = getBatch(batchId);
    if (!batch) {
        return;
    }
    
    uint32_t instanceCount = m_config.frustumCulling || m_config.distanceCulling
        ? static_cast<uint32_t>(batch->getVisibleInstances().size())
        : batch->getInstanceCount();
    
    if (instanceCount > 0) {
        // TODO: Actual draw call
        m_stats.drawCalls++;
        m_stats.instancesRendered += instanceCount;
    }
}

void GPUInstancingSystem::resetStatistics() {
    m_stats = InstancingStatistics();
    m_stats.totalInstances = 0;
    for (const auto& pair : m_batches) {
        m_stats.totalInstances += pair.second->getInstanceCount();
    }
}

void GPUInstancingSystem::dumpBatchInfo() const {
    std::cout << "=== GPU Instancing System ===" << std::endl;
    std::cout << "Total Batches: " << m_batches.size() << std::endl;
    std::cout << "Total Instances: " << m_stats.totalInstances << std::endl;
    std::cout << "Instances Rendered: " << m_stats.instancesRendered << std::endl;
    std::cout << "Instances Culled: " << m_stats.instancesCulled << std::endl;
    std::cout << "Draw Calls: " << m_stats.drawCalls << std::endl;
    
    for (const auto& pair : m_batches) {
        const InstanceBatch* batch = pair.second.get();
        std::cout << "  Batch " << pair.first 
                  << " (Mesh " << batch->getMeshId() << "): "
                  << batch->getInstanceCount() << " instances" << std::endl;
    }
}

void GPUInstancingSystem::updateBuffers() {
    for (auto& pair : m_batches) {
        if (pair.second->needsUpdate()) {
            pair.second->updateBuffer();
        }
    }
}

void GPUInstancingSystem::performCulling(const float* frustumPlanes, const float* cameraPosition) {
    updateCulling(frustumPlanes, cameraPosition);
}

// InstanceDataBuilder Implementation
InstanceDataBuilder::InstanceDataBuilder() {
}

InstanceDataBuilder& InstanceDataBuilder::setTransform(const float* matrix) {
    std::memcpy(m_data.transform, matrix, 16 * sizeof(float));
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setPosition(float x, float y, float z) {
    m_data.transform[12] = x;
    m_data.transform[13] = y;
    m_data.transform[14] = z;
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setRotation(float x, float y, float z, float w) {
    // Convert quaternion to rotation matrix (simplified)
    // This is a placeholder - proper implementation would convert quaternion to matrix
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setScale(float x, float y, float z) {
    m_data.transform[0] *= x;
    m_data.transform[5] *= y;
    m_data.transform[10] *= z;
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setColor(float r, float g, float b, float a) {
    m_data.color[0] = r;
    m_data.color[1] = g;
    m_data.color[2] = b;
    m_data.color[3] = a;
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setCustomData(const float* data) {
    std::memcpy(m_data.customData, data, 4 * sizeof(float));
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setTextureIndex(uint32_t index) {
    m_data.textureIndex = index;
    return *this;
}

InstanceDataBuilder& InstanceDataBuilder::setMaterialId(uint32_t id) {
    m_data.materialId = id;
    return *this;
}

InstanceData InstanceDataBuilder::build() const {
    return m_data;
}

} // namespace Graphics
} // namespace JJM
