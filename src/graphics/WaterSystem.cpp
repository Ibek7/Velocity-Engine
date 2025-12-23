#include "graphics/WaterSystem.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace Engine {

// WaterPlane implementation
WaterPlane::WaterPlane(float width, float depth, int gridResolution)
    : m_width(width)
    , m_depth(depth)
    , m_gridResolution(gridResolution)
    , m_waveAmplitude(0.3f)
    , m_waveFrequency(1.0f)
    , m_waveSpeed(1.0f)
    , m_time(0.0f)
    , m_reflectionStrength(0.5f)
    , m_refractionStrength(0.3f)
    , m_foamAmount(0.2f)
{
    m_position[0] = 0.0f;
    m_position[1] = 0.0f;
    m_position[2] = 0.0f;
    
    m_waterColor[0] = 0.0f;
    m_waterColor[1] = 0.3f;
    m_waterColor[2] = 0.5f;
    m_waterColor[3] = 0.7f;
    
    generateMesh();
}

WaterPlane::~WaterPlane() {
}

void WaterPlane::generateMesh() {
    m_vertices.clear();
    m_indices.clear();
    
    float stepX = m_width / m_gridResolution;
    float stepZ = m_depth / m_gridResolution;
    
    // Generate vertices
    for (int z = 0; z <= m_gridResolution; ++z) {
        for (int x = 0; x <= m_gridResolution; ++x) {
            WaterVertex vertex;
            
            vertex.position[0] = x * stepX - m_width * 0.5f;
            vertex.position[1] = 0.0f;
            vertex.position[2] = z * stepZ - m_depth * 0.5f;
            
            vertex.normal[0] = 0.0f;
            vertex.normal[1] = 1.0f;
            vertex.normal[2] = 0.0f;
            
            vertex.texCoord[0] = static_cast<float>(x) / m_gridResolution;
            vertex.texCoord[1] = static_cast<float>(z) / m_gridResolution;
            
            vertex.tangent[0] = 1.0f;
            vertex.tangent[1] = 0.0f;
            vertex.tangent[2] = 0.0f;
            
            m_vertices.push_back(vertex);
        }
    }
    
    // Generate indices
    for (int z = 0; z < m_gridResolution; ++z) {
        for (int x = 0; x < m_gridResolution; ++x) {
            int topLeft = z * (m_gridResolution + 1) + x;
            int topRight = topLeft + 1;
            int bottomLeft = (z + 1) * (m_gridResolution + 1) + x;
            int bottomRight = bottomLeft + 1;
            
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
}

void WaterPlane::update(float deltaTime) {
    m_time += deltaTime * m_waveSpeed;
    updateWaves(m_time);
    
    // Update ripples
    for (auto& ripple : m_ripples) {
        ripple.radius += deltaTime * 2.0f;
        ripple.strength *= 0.95f; // Fade out
    }
    
    // Remove dead ripples
    m_ripples.erase(
        std::remove_if(m_ripples.begin(), m_ripples.end(),
            [](const Ripple& r) { return r.radius >= r.maxRadius || r.strength < 0.01f; }),
        m_ripples.end()
    );
}

void WaterPlane::updateWaves(float time) {
    // Update vertex positions and normals based on wave function
    for (auto& vertex : m_vertices) {
        float worldX = vertex.position[0] + m_position[0];
        float worldZ = vertex.position[2] + m_position[2];
        
        float height = calculateWaveHeight(worldX, worldZ, time);
        vertex.position[1] = height;
        
        // Calculate normal using finite differences
        float delta = 0.1f;
        float heightX = calculateWaveHeight(worldX + delta, worldZ, time);
        float heightZ = calculateWaveHeight(worldX, worldZ + delta, time);
        
        float dx = heightX - height;
        float dz = heightZ - height;
        
        // Normalize
        float len = std::sqrt(delta*delta + dx*dx + dz*dz);
        if (len > 0.0f) {
            vertex.normal[0] = -dx / len;
            vertex.normal[1] = delta / len;
            vertex.normal[2] = -dz / len;
        }
    }
}

float WaterPlane::calculateWaveHeight(float x, float z, float time) const {
    // Gerstner wave function
    float height = 0.0f;
    
    // Wave 1
    float k1 = m_waveFrequency * 0.8f;
    float w1 = std::sqrt(9.8f * k1);
    height += m_waveAmplitude * std::sin(k1 * x + w1 * time);
    
    // Wave 2 (perpendicular)
    float k2 = m_waveFrequency * 0.6f;
    float w2 = std::sqrt(9.8f * k2);
    height += m_waveAmplitude * 0.5f * std::sin(k2 * z + w2 * time + 1.5f);
    
    // Wave 3 (diagonal)
    float k3 = m_waveFrequency;
    float w3 = std::sqrt(9.8f * k3);
    height += m_waveAmplitude * 0.3f * std::sin(k3 * (x + z) * 0.707f + w3 * time + 3.0f);
    
    // Add ripples
    for (const auto& ripple : m_ripples) {
        float dx = x - ripple.x;
        float dz = z - ripple.z;
        float dist = std::sqrt(dx*dx + dz*dz);
        
        if (dist < ripple.radius && dist < ripple.maxRadius) {
            float t = dist / ripple.radius;
            height += ripple.strength * std::sin(t * 6.28f) * (1.0f - t);
        }
    }
    
    return height;
}

void WaterPlane::createRipple(float worldX, float worldZ, float strength) {
    Ripple ripple;
    ripple.x = worldX - m_position[0];
    ripple.z = worldZ - m_position[2];
    ripple.strength = strength;
    ripple.radius = 0.0f;
    ripple.maxRadius = 5.0f + strength * 2.0f;
    
    m_ripples.push_back(ripple);
}

float WaterPlane::getHeightAt(float worldX, float worldZ) const {
    float localX = worldX - m_position[0];
    float localZ = worldZ - m_position[2];
    
    return m_position[1] + calculateWaveHeight(worldX, worldZ, m_time);
}

void WaterPlane::setPosition(float x, float y, float z) {
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;
}

void WaterPlane::setWaterColor(float r, float g, float b, float a) {
    m_waterColor[0] = r;
    m_waterColor[1] = g;
    m_waterColor[2] = b;
    m_waterColor[3] = a;
}

void WaterPlane::setReflectionStrength(float strength) {
    m_reflectionStrength = std::max(0.0f, std::min(1.0f, strength));
}

void WaterPlane::setRefractionStrength(float strength) {
    m_refractionStrength = std::max(0.0f, std::min(1.0f, strength));
}

void WaterPlane::setFoamAmount(float amount) {
    m_foamAmount = std::max(0.0f, std::min(1.0f, amount));
}

void WaterPlane::render() {
    // TODO: Render water mesh with shaders
}

// WaterSystem implementation
WaterSystem::WaterSystem()
    : m_quality(WaterQuality::Medium)
    , m_globalWaveScale(1.0f)
    , m_enableReflections(true)
    , m_enableRefractions(true)
    , m_enableFoam(true)
{
}

WaterSystem& WaterSystem::getInstance() {
    static WaterSystem instance;
    return instance;
}

void WaterSystem::addWaterPlane(float width, float depth, int resolution, float x, float y, float z) {
    auto plane = std::make_unique<WaterPlane>(width, depth, resolution);
    plane->setPosition(x, y, z);
    
    // Adjust settings based on quality
    switch (m_quality) {
        case WaterQuality::Low:
            plane->setWaveAmplitude(0.2f);
            break;
        case WaterQuality::Medium:
            plane->setWaveAmplitude(0.3f);
            break;
        case WaterQuality::High:
            plane->setWaveAmplitude(0.4f);
            break;
        case WaterQuality::Ultra:
            plane->setWaveAmplitude(0.5f);
            break;
    }
    
    m_waterPlanes.push_back(std::move(plane));
}

void WaterSystem::removeWaterPlane(float x, float y, float z) {
    m_waterPlanes.erase(
        std::remove_if(m_waterPlanes.begin(), m_waterPlanes.end(),
            [x, y, z](const std::unique_ptr<WaterPlane>& plane) {
                (void)plane; (void)x; (void)y; (void)z;
                // TODO: Check position match
                return false;
            }),
        m_waterPlanes.end()
    );
}

void WaterSystem::clearWaterPlanes() {
    m_waterPlanes.clear();
}

void WaterSystem::update(float deltaTime) {
    for (auto& plane : m_waterPlanes) {
        plane->update(deltaTime * m_globalWaveScale);
    }
}

void WaterSystem::render() {
    for (auto& plane : m_waterPlanes) {
        plane->render();
    }
}

void WaterSystem::setQuality(WaterQuality quality) {
    m_quality = quality;
}

void WaterSystem::createSplash(float x, float y, float z, float strength) {
    for (auto& plane : m_waterPlanes) {
        plane->createRipple(x, z, strength);
    }
    (void)y; // Y coordinate not used for splash creation
}

float WaterSystem::getWaterHeightAt(float x, float z) const {
    for (const auto& plane : m_waterPlanes) {
        // Return first matching plane height
        return plane->getHeightAt(x, z);
    }
    return 0.0f;
}

bool WaterSystem::isUnderwater(float x, float y, float z) const {
    float waterHeight = getWaterHeightAt(x, z);
    return y < waterHeight;
}

} // namespace Engine
