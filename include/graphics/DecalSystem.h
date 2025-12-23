#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Decal rendering system for bullet holes, blood splats, etc.
namespace Engine {

enum class DecalType {
    BulletHole,
    BloodSplat,
    Scorch,
    Dirt,
    Custom
};

struct DecalVertex {
    float position[3];
    float normal[3];
    float texCoord[2];
};

class Decal {
public:
    Decal(DecalType type, const float* position, const float* normal, float size);
    ~Decal();

    // Rendering
    void render();
    
    // Properties
    void setTexture(const std::string& texturePath);
    void setColor(float r, float g, float b, float a);
    void setSize(float size) { m_size = size; }
    void setFadeTime(float seconds) { m_fadeTime = seconds; }
    
    // Lifecycle
    void update(float deltaTime);
    bool shouldRemove() const { return m_age >= m_fadeTime; }
    float getAge() const { return m_age; }
    
    DecalType getType() const { return m_type; }

private:
    void generateGeometry(const float* position, const float* normal, float size);
    void projectOntoSurface();

    DecalType m_type;
    std::vector<DecalVertex> m_vertices;
    std::vector<uint16_t> m_indices;
    
    std::string m_texturePath;
    float m_color[4];
    float m_size;
    float m_age;
    float m_fadeTime;
    
    float m_position[3];
    float m_normal[3];
};

class DecalSystem {
public:
    static DecalSystem& getInstance();

    // Decal creation
    void addDecal(DecalType type, const float* position, const float* normal, float size);
    void addBulletHole(const float* position, const float* normal);
    void addBloodSplat(const float* position, const float* normal);
    void addScorchMark(const float* position, const float* normal);
    
    // Update and render
    void update(float deltaTime);
    void render();
    void clear();
    
    // Configuration
    void setMaxDecals(size_t max) { m_maxDecals = max; }
    void setDefaultFadeTime(float seconds) { m_defaultFadeTime = seconds; }
    void setDecalTexture(DecalType type, const std::string& path);
    
    size_t getDecalCount() const { return m_decals.size(); }

private:
    DecalSystem();
    DecalSystem(const DecalSystem&) = delete;
    DecalSystem& operator=(const DecalSystem&) = delete;

    void removeOldDecals();

    std::vector<std::unique_ptr<Decal>> m_decals;
    size_t m_maxDecals;
    float m_defaultFadeTime;
};

} // namespace Engine
