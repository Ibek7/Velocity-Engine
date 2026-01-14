#include "graphics/TerrainSystem.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Graphics {

// TerrainChunk Implementation
TerrainChunk::TerrainChunk(int32_t x, int32_t z, uint32_t size)
    : m_x(x)
    , m_z(z)
    , m_size(size)
    , m_currentLOD(0)
    , m_vbo(0)
    , m_ibo(0)
    , m_indexCount(0)
    , m_visible(true) {
}

TerrainChunk::~TerrainChunk() {
    // TODO: Release GPU buffers
}

void TerrainChunk::generateMesh(const std::vector<float>& heightMap, uint32_t lod) {
    m_vertices.clear();
    m_indices.clear();
    
    uint32_t step = 1 << lod; // 2^lod
    uint32_t gridSize = m_size / step;
    
    // Generate vertices
    for (uint32_t z = 0; z <= gridSize; ++z) {
        for (uint32_t x = 0; x <= gridSize; ++x) {
            float worldX = static_cast<float>(m_x + x * step);
            float worldZ = static_cast<float>(m_z + z * step);
            
            // Sample heightmap (simplified)
            float height = 0.0f;
            // TODO: Properly sample from heightMap
            
            // Position
            m_vertices.push_back(worldX);
            m_vertices.push_back(height);
            m_vertices.push_back(worldZ);
            
            // Normal (placeholder)
            m_vertices.push_back(0.0f);
            m_vertices.push_back(1.0f);
            m_vertices.push_back(0.0f);
            
            // UV
            m_vertices.push_back(static_cast<float>(x) / gridSize);
            m_vertices.push_back(static_cast<float>(z) / gridSize);
        }
    }
    
    // Generate indices
    for (uint32_t z = 0; z < gridSize; ++z) {
        for (uint32_t x = 0; x < gridSize; ++x) {
            uint32_t topLeft = z * (gridSize + 1) + x;
            uint32_t topRight = topLeft + 1;
            uint32_t bottomLeft = (z + 1) * (gridSize + 1) + x;
            uint32_t bottomRight = bottomLeft + 1;
            
            // First triangle
            m_indices.push_back(topLeft);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(topRight);
            
            // Second triangle
            m_indices.push_back(topRight);
            m_indices.push_back(bottomLeft);
            m_indices.push_back(bottomRight);
        }
    }
    
    m_indexCount = static_cast<uint32_t>(m_indices.size());
}

void TerrainChunk::updateBuffers() {
    // TODO: Upload to GPU
}

// TerrainSystem Implementation
TerrainSystem::TerrainSystem()
    : m_splatMapTexture(0)
    , m_terrainShader(0)
    , m_wireframe(false)
    , m_visualizeLOD(false) {
}

TerrainSystem::~TerrainSystem() {
    m_chunks.clear();
}

void TerrainSystem::configure(const TerrainConfig& config) {
    m_config = config;
    
    // Resize heightmap
    uint32_t heightMapSize = config.heightMapResolution * config.heightMapResolution;
    m_heightMap.resize(heightMapSize, config.baseHeight);
}

void TerrainSystem::setLODConfig(const TerrainLODConfig& lodConfig) {
    m_lodConfig = lodConfig;
}

bool TerrainSystem::loadHeightMap(const std::string& filepath) {
    // TODO: Load heightmap from image file
    std::cout << "Loading heightmap from: " << filepath << std::endl;
    return false;
}

void TerrainSystem::generateHeightMap(uint32_t seed) {
    std::vector<ProceduralTerrainGenerator::NoiseLayer> layers;
    
    // Add base layer
    ProceduralTerrainGenerator::NoiseLayer baseLayer;
    baseLayer.type = ProceduralTerrainGenerator::NoiseType::Perlin;
    baseLayer.frequency = 0.5f;
    baseLayer.amplitude = 50.0f;
    baseLayer.octaves = 6;
    layers.push_back(baseLayer);
    
    // Add detail layer
    ProceduralTerrainGenerator::NoiseLayer detailLayer;
    detailLayer.type = ProceduralTerrainGenerator::NoiseType::Perlin;
    detailLayer.frequency = 2.0f;
    detailLayer.amplitude = 10.0f;
    detailLayer.octaves = 4;
    layers.push_back(detailLayer);
    
    ProceduralTerrainGenerator::generateHeightMap(
        m_heightMap,
        m_config.heightMapResolution,
        m_config.heightMapResolution,
        seed,
        layers
    );
}

void TerrainSystem::setHeight(uint32_t x, uint32_t z, float height) {
    if (x >= m_config.heightMapResolution || z >= m_config.heightMapResolution) {
        return;
    }
    
    uint32_t index = z * m_config.heightMapResolution + x;
    m_heightMap[index] = height;
}

float TerrainSystem::getHeight(float x, float z) const {
    // Convert world coordinates to heightmap coordinates
    float u = x / m_config.width * m_config.heightMapResolution;
    float v = z / m_config.depth * m_config.heightMapResolution;
    
    // Clamp to valid range
    u = std::clamp(u, 0.0f, static_cast<float>(m_config.heightMapResolution - 1));
    v = std::clamp(v, 0.0f, static_cast<float>(m_config.heightMapResolution - 1));
    
    // Bilinear interpolation
    uint32_t x0 = static_cast<uint32_t>(u);
    uint32_t z0 = static_cast<uint32_t>(v);
    uint32_t x1 = std::min(x0 + 1, m_config.heightMapResolution - 1);
    uint32_t z1 = std::min(z0 + 1, m_config.heightMapResolution - 1);
    
    float fx = u - x0;
    float fz = v - z0;
    
    float h00 = m_heightMap[z0 * m_config.heightMapResolution + x0];
    float h10 = m_heightMap[z0 * m_config.heightMapResolution + x1];
    float h01 = m_heightMap[z1 * m_config.heightMapResolution + x0];
    float h11 = m_heightMap[z1 * m_config.heightMapResolution + x1];
    
    float h0 = h00 * (1.0f - fx) + h10 * fx;
    float h1 = h01 * (1.0f - fx) + h11 * fx;
    
    return h0 * (1.0f - fz) + h1 * fz;
}

void TerrainSystem::smoothHeightMap(uint32_t iterations) {
    std::vector<float> temp = m_heightMap;
    
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        for (uint32_t z = 1; z < m_config.heightMapResolution - 1; ++z) {
            for (uint32_t x = 1; x < m_config.heightMapResolution - 1; ++x) {
                uint32_t idx = z * m_config.heightMapResolution + x;
                
                // Average with neighbors
                float sum = 0.0f;
                sum += m_heightMap[idx - 1];
                sum += m_heightMap[idx + 1];
                sum += m_heightMap[idx - m_config.heightMapResolution];
                sum += m_heightMap[idx + m_config.heightMapResolution];
                sum += m_heightMap[idx] * 4.0f;
                
                temp[idx] = sum / 8.0f;
            }
        }
        
        m_heightMap = temp;
    }
}

void TerrainSystem::generateTerrain() {
    createChunks();
}

void TerrainSystem::createChunks() {
    m_chunks.clear();
    
    uint32_t chunksX = m_config.width / m_config.chunkSize;
    uint32_t chunksZ = m_config.depth / m_config.chunkSize;
    
    for (uint32_t z = 0; z < chunksZ; ++z) {
        for (uint32_t x = 0; x < chunksX; ++x) {
            int32_t chunkX = x * m_config.chunkSize;
            int32_t chunkZ = z * m_config.chunkSize;
            
            auto chunk = std::make_unique<TerrainChunk>(chunkX, chunkZ, m_config.chunkSize);
            chunk->generateMesh(m_heightMap, 0);
            chunk->updateBuffers();
            
            m_chunks.push_back(std::move(chunk));
        }
    }
}

void TerrainSystem::updateChunks(const float* cameraPosition) {
    updateChunkLODs(cameraPosition);
}

void TerrainSystem::updateChunkLODs(const float* cameraPosition) {
    for (auto& chunk : m_chunks) {
        float distance = calculateChunkDistance(chunk.get(), cameraPosition);
        uint32_t lod = selectLOD(distance);
        
        if (lod != chunk->getCurrentLOD()) {
            chunk->setCurrentLOD(lod);
            chunk->generateMesh(m_heightMap, lod);
            chunk->updateBuffers();
        }
    }
}

float TerrainSystem::calculateChunkDistance(const TerrainChunk* chunk, const float* cameraPosition) const {
    float chunkCenterX = chunk->getX() + m_config.chunkSize * 0.5f;
    float chunkCenterZ = chunk->getZ() + m_config.chunkSize * 0.5f;
    
    float dx = chunkCenterX - cameraPosition[0];
    float dz = chunkCenterZ - cameraPosition[2];
    
    return std::sqrt(dx * dx + dz * dz);
}

uint32_t TerrainSystem::selectLOD(float distance) const {
    for (uint32_t lod = 0; lod < m_lodConfig.lodLevels; ++lod) {
        if (distance < m_lodConfig.lodDistance * (lod + 1)) {
            return lod;
        }
    }
    return m_lodConfig.lodLevels - 1;
}

void TerrainSystem::addLayer(const TerrainLayer& layer) {
    m_layers.push_back(layer);
}

void TerrainSystem::removeLayer(uint32_t index) {
    if (index < m_layers.size()) {
        m_layers.erase(m_layers.begin() + index);
    }
}

const TerrainLayer* TerrainSystem::getLayer(uint32_t index) const {
    if (index >= m_layers.size()) {
        return nullptr;
    }
    return &m_layers[index];
}

void TerrainSystem::generateSplatMap() {
    // TODO: Generate splatmap texture based on height and slope
}

void TerrainSystem::render(const float* viewMatrix, const float* projectionMatrix) {
    for (auto& chunk : m_chunks) {
        if (chunk->isVisible()) {
            renderChunk(chunk.get());
        }
    }
}

void TerrainSystem::renderChunk(TerrainChunk* chunk) {
    // TODO: Render chunk with appropriate shader
}

void TerrainSystem::getNormal(float x, float z, float* normal) const {
    const float offset = 1.0f;
    
    float hL = getHeight(x - offset, z);
    float hR = getHeight(x + offset, z);
    float hD = getHeight(x, z - offset);
    float hU = getHeight(x, z + offset);
    
    normal[0] = hL - hR;
    normal[1] = 2.0f * offset;
    normal[2] = hD - hU;
    
    // Normalize
    float len = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);
    if (len > 0.0f) {
        normal[0] /= len;
        normal[1] /= len;
        normal[2] /= len;
    }
}

bool TerrainSystem::raycast(const float* origin, const float* direction, float* hitPoint) const {
    // TODO: Implement terrain raycasting
    return false;
}

// ProceduralTerrainGenerator Implementation
void ProceduralTerrainGenerator::generateHeightMap(
    std::vector<float>& heightMap,
    uint32_t width, uint32_t height,
    uint32_t seed,
    const std::vector<NoiseLayer>& layers) {
    
    heightMap.resize(width * height, 0.0f);
    
    // Simple placeholder noise generation
    for (uint32_t z = 0; z < height; ++z) {
        for (uint32_t x = 0; x < width; ++x) {
            float value = 0.0f;
            
            for (const auto& layer : layers) {
                // Simplified Perlin-like noise
                float nx = x * layer.frequency / width;
                float nz = z * layer.frequency / height;
                
                float noiseValue = std::sin(nx * 10.0f + seed) * std::cos(nz * 10.0f + seed);
                noiseValue = (noiseValue + 1.0f) * 0.5f; // Normalize to 0-1
                
                value += noiseValue * layer.amplitude;
            }
            
            heightMap[z * width + x] = value;
        }
    }
}

void ProceduralTerrainGenerator::applyErosion(
    std::vector<float>& heightMap,
    uint32_t width, uint32_t height,
    uint32_t iterations,
    float erosionStrength) {
    
    // Simplified hydraulic erosion simulation
    for (uint32_t iter = 0; iter < iterations; ++iter) {
        // TODO: Implement proper erosion algorithm
    }
}

void ProceduralTerrainGenerator::applyTerracing(
    std::vector<float>& heightMap,
    uint32_t levels,
    float smoothness) {
    
    for (auto& height : heightMap) {
        float terrace = std::floor(height * levels) / levels;
        height = height * (1.0f - smoothness) + terrace * smoothness;
    }
}

} // namespace Graphics
} // namespace JJM
