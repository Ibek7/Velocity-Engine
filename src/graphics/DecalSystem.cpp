#include "graphics/DecalSystem.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace Engine {

// Decal implementation
Decal::Decal(DecalType type, const float* position, const float* normal, float size)
    : m_type(type)
    , m_size(size)
    , m_age(0.0f)
    , m_fadeTime(10.0f)
{
    std::memcpy(m_position, position, sizeof(float) * 3);
    std::memcpy(m_normal, normal, sizeof(float) * 3);
    
    m_color[0] = 1.0f;
    m_color[1] = 1.0f;
    m_color[2] = 1.0f;
    m_color[3] = 1.0f;
    
    generateGeometry(position, normal, size);
}

Decal::~Decal() {
}

void Decal::generateGeometry(const float* position, const float* normal, float size) {
    // Create tangent and bitangent vectors
    float tangent[3], bitangent[3];
    
    // Find a vector not parallel to normal
    float up[3] = {0.0f, 1.0f, 0.0f};
    if (std::abs(normal[1]) > 0.9f) {
        up[0] = 1.0f;
        up[1] = 0.0f;
        up[2] = 0.0f;
    }
    
    // Compute tangent (cross product of normal and up)
    tangent[0] = normal[1] * up[2] - normal[2] * up[1];
    tangent[1] = normal[2] * up[0] - normal[0] * up[2];
    tangent[2] = normal[0] * up[1] - normal[1] * up[0];
    
    // Normalize tangent
    float len = std::sqrt(tangent[0]*tangent[0] + tangent[1]*tangent[1] + tangent[2]*tangent[2]);
    if (len > 0.0f) {
        tangent[0] /= len;
        tangent[1] /= len;
        tangent[2] /= len;
    }
    
    // Compute bitangent (cross product of normal and tangent)
    bitangent[0] = normal[1] * tangent[2] - normal[2] * tangent[1];
    bitangent[1] = normal[2] * tangent[0] - normal[0] * tangent[2];
    bitangent[2] = normal[0] * tangent[1] - normal[1] * tangent[0];
    
    // Create quad vertices
    float halfSize = size * 0.5f;
    
    DecalVertex vertices[4];
    
    // Bottom-left
    vertices[0].position[0] = position[0] - tangent[0] * halfSize - bitangent[0] * halfSize;
    vertices[0].position[1] = position[1] - tangent[1] * halfSize - bitangent[1] * halfSize;
    vertices[0].position[2] = position[2] - tangent[2] * halfSize - bitangent[2] * halfSize;
    vertices[0].normal[0] = normal[0];
    vertices[0].normal[1] = normal[1];
    vertices[0].normal[2] = normal[2];
    vertices[0].texCoord[0] = 0.0f;
    vertices[0].texCoord[1] = 0.0f;
    
    // Bottom-right
    vertices[1].position[0] = position[0] + tangent[0] * halfSize - bitangent[0] * halfSize;
    vertices[1].position[1] = position[1] + tangent[1] * halfSize - bitangent[1] * halfSize;
    vertices[1].position[2] = position[2] + tangent[2] * halfSize - bitangent[2] * halfSize;
    vertices[1].normal[0] = normal[0];
    vertices[1].normal[1] = normal[1];
    vertices[1].normal[2] = normal[2];
    vertices[1].texCoord[0] = 1.0f;
    vertices[1].texCoord[1] = 0.0f;
    
    // Top-right
    vertices[2].position[0] = position[0] + tangent[0] * halfSize + bitangent[0] * halfSize;
    vertices[2].position[1] = position[1] + tangent[1] * halfSize + bitangent[1] * halfSize;
    vertices[2].position[2] = position[2] + tangent[2] * halfSize + bitangent[2] * halfSize;
    vertices[2].normal[0] = normal[0];
    vertices[2].normal[1] = normal[1];
    vertices[2].normal[2] = normal[2];
    vertices[2].texCoord[0] = 1.0f;
    vertices[2].texCoord[1] = 1.0f;
    
    // Top-left
    vertices[3].position[0] = position[0] - tangent[0] * halfSize + bitangent[0] * halfSize;
    vertices[3].position[1] = position[1] - tangent[1] * halfSize + bitangent[1] * halfSize;
    vertices[3].position[2] = position[2] - tangent[2] * halfSize + bitangent[2] * halfSize;
    vertices[3].normal[0] = normal[0];
    vertices[3].normal[1] = normal[1];
    vertices[3].normal[2] = normal[2];
    vertices[3].texCoord[0] = 0.0f;
    vertices[3].texCoord[1] = 1.0f;
    
    m_vertices.assign(vertices, vertices + 4);
    
    // Create indices for two triangles
    uint16_t indices[] = {0, 1, 2, 0, 2, 3};
    m_indices.assign(indices, indices + 6);
}

void Decal::projectOntoSurface() {
    // TODO: Ray cast and project decal onto nearby surfaces
}

void Decal::setTexture(const std::string& texturePath) {
    m_texturePath = texturePath;
}

void Decal::setColor(float r, float g, float b, float a) {
    m_color[0] = r;
    m_color[1] = g;
    m_color[2] = b;
    m_color[3] = a;
}

void Decal::update(float deltaTime) {
    m_age += deltaTime;
    
    // Fade out near end of life
    if (m_fadeTime > 0.0f) {
        float fadeRatio = m_age / m_fadeTime;
        if (fadeRatio > 0.7f) {
            float fadeAmount = (fadeRatio - 0.7f) / 0.3f;
            m_color[3] = 1.0f - fadeAmount;
        }
    }
}

void Decal::render() {
    // TODO: Bind texture and render quad
}

// DecalSystem implementation
DecalSystem::DecalSystem()
    : m_maxDecals(500)
    , m_defaultFadeTime(30.0f)
{
}

DecalSystem& DecalSystem::getInstance() {
    static DecalSystem instance;
    return instance;
}

void DecalSystem::addDecal(DecalType type, const float* position, const float* normal, float size) {
    auto decal = std::make_unique<Decal>(type, position, normal, size);
    decal->setFadeTime(m_defaultFadeTime);
    
    m_decals.push_back(std::move(decal));
    
    // Remove oldest decals if over limit
    if (m_decals.size() > m_maxDecals) {
        m_decals.erase(m_decals.begin());
    }
}

void DecalSystem::addBulletHole(const float* position, const float* normal) {
    addDecal(DecalType::BulletHole, position, normal, 0.1f);
}

void DecalSystem::addBloodSplat(const float* position, const float* normal) {
    addDecal(DecalType::BloodSplat, position, normal, 0.3f);
}

void DecalSystem::addScorchMark(const float* position, const float* normal) {
    addDecal(DecalType::Scorch, position, normal, 0.5f);
}

void DecalSystem::update(float deltaTime) {
    // Update all decals
    for (auto& decal : m_decals) {
        decal->update(deltaTime);
    }
    
    // Remove expired decals
    removeOldDecals();
}

void DecalSystem::render() {
    for (auto& decal : m_decals) {
        decal->render();
    }
}

void DecalSystem::clear() {
    m_decals.clear();
}

void DecalSystem::removeOldDecals() {
    m_decals.erase(
        std::remove_if(m_decals.begin(), m_decals.end(),
            [](const std::unique_ptr<Decal>& decal) {
                return decal->shouldRemove();
            }),
        m_decals.end()
    );
}

void DecalSystem::setDecalTexture(DecalType type, const std::string& path) {
    // TODO: Store texture paths for each decal type
    (void)type;
    (void)path;
}

} // namespace Engine
