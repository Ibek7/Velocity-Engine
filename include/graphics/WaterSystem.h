#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Water simulation system with waves and reflections
namespace Engine {

struct WaterVertex {
    float position[3];
    float normal[3];
    float texCoord[2];
    float tangent[3];
};

enum class WaterQuality {
    Low,
    Medium,
    High,
    Ultra
};

class WaterPlane {
public:
    WaterPlane(float width, float depth, int gridResolution);
    ~WaterPlane();

    // Rendering
    void render();
    void update(float deltaTime);
    
    // Configuration
    void setPosition(float x, float y, float z);
    void setWaveAmplitude(float amplitude) { m_waveAmplitude = amplitude; }
    void setWaveFrequency(float frequency) { m_waveFrequency = frequency; }
    void setWaveSpeed(float speed) { m_waveSpeed = speed; }
    void setWaterColor(float r, float g, float b, float a);
    void setReflectionStrength(float strength);
    void setRefractionStrength(float strength);
    void setFoamAmount(float amount);
    
    // Interaction
    void createRipple(float worldX, float worldZ, float strength);
    float getHeightAt(float worldX, float worldZ) const;

private:
    void generateMesh();
    void updateWaves(float time);
    float calculateWaveHeight(float x, float z, float time) const;

    float m_width;
    float m_depth;
    int m_gridResolution;
    
    std::vector<WaterVertex> m_vertices;
    std::vector<uint32_t> m_indices;
    
    float m_position[3];
    float m_waveAmplitude;
    float m_waveFrequency;
    float m_waveSpeed;
    float m_time;
    
    float m_waterColor[4];
    float m_reflectionStrength;
    float m_refractionStrength;
    float m_foamAmount;
    
    struct Ripple {
        float x, z;
        float strength;
        float radius;
        float maxRadius;
    };
    std::vector<Ripple> m_ripples;
};

class WaterSystem {
public:
    static WaterSystem& getInstance();

    // Water plane management
    void addWaterPlane(float width, float depth, int resolution, float x, float y, float z);
    void removeWaterPlane(float x, float y, float z);
    void clearWaterPlanes();
    
    // Update and render
    void update(float deltaTime);
    void render();
    
    // Global settings
    void setQuality(WaterQuality quality);
    void setGlobalWaveScale(float scale) { m_globalWaveScale = scale; }
    void setEnableReflections(bool enable) { m_enableReflections = enable; }
    void setEnableRefractions(bool enable) { m_enableRefractions = enable; }
    void setEnableFoam(bool enable) { m_enableFoam = enable; }
    
    // Interaction
    void createSplash(float x, float y, float z, float strength);
    float getWaterHeightAt(float x, float z) const;
    bool isUnderwater(float x, float y, float z) const;

private:
    WaterSystem();
    WaterSystem(const WaterSystem&) = delete;
    WaterSystem& operator=(const WaterSystem&) = delete;

    std::vector<std::unique_ptr<WaterPlane>> m_waterPlanes;
    
    WaterQuality m_quality;
    float m_globalWaveScale;
    bool m_enableReflections;
    bool m_enableRefractions;
    bool m_enableFoam;
};

} // namespace Engine
