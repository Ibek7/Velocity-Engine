#pragma once

#include <string>
#include <vector>
#include <map>

// Volumetric fog/clouds rendering
namespace Engine {

struct VolumetricFogSettings {
    float density;
    float heightFalloff;
    float baseHeight;
    float maxHeight;
    float scatteringCoefficient;
    float absorptionCoefficient;
    unsigned int color;
    bool useHeightFog;
};

class VolumetricFog {
public:
    static VolumetricFog& getInstance();

    // Setup
    void initialize(int width, int height, int depth);
    void shutdown();
    
    // Rendering
    void render(unsigned int depthTexture, unsigned int lightBuffer);
    unsigned int getFogTexture() const { return m_fogTexture; }
    
    // Settings
    void setDensity(float density);
    void setHeightFalloff(float falloff);
    void setHeightRange(float baseHeight, float maxHeight);
    void setColor(unsigned int color);
    void setScattering(float scattering);
    void setAbsorption(float absorption);
    
    // Quality
    void setVolumeResolution(int width, int height, int depth);
    void setSampleCount(int count) { m_sampleCount = count; }
    void setTemporalBlendFactor(float factor);
    
    // Features
    void enableHeightFog(bool enable);
    void enableVolumetricScattering(bool enable) { m_volumetricScattering = enable; }
    void enableTemporalReprojection(bool enable) { m_temporalReprojection = enable; }
    
    // Animation
    void setWindSpeed(float x, float y, float z);
    void update(float deltaTime);
    
    // Toggles
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    bool isEnabled() const { return m_enabled; }

private:
    VolumetricFog();
    VolumetricFog(const VolumetricFog&) = delete;
    VolumetricFog& operator=(const VolumetricFog&) = delete;

    void createVolumeTexture();
    void destroyVolumeTexture();
    void rayMarch();
    void applyTemporalFilter();

    int m_volumeWidth;
    int m_volumeHeight;
    int m_volumeDepth;
    
    unsigned int m_fogTexture;
    unsigned int m_volumeTexture;
    unsigned int m_framebuffer;
    unsigned int m_shader;
    
    VolumetricFogSettings m_settings;
    
    int m_sampleCount;
    float m_temporalBlendFactor;
    bool m_volumetricScattering;
    bool m_temporalReprojection;
    
    float m_windSpeedX;
    float m_windSpeedY;
    float m_windSpeedZ;
    float m_time;
    
    bool m_enabled;
};

// Volumetric clouds
class VolumetricClouds {
public:
    static VolumetricClouds& getInstance();

    // Setup
    void initialize(int width, int height);
    void shutdown();
    
    // Rendering
    void render(unsigned int depthTexture);
    unsigned int getCloudTexture() const { return m_cloudTexture; }
    
    // Cloud properties
    void setCoverage(float coverage);
    void setDensity(float density);
    void setScale(float scale);
    void setAltitude(float minAltitude, float maxAltitude);
    
    // Lighting
    void setSunDirection(float x, float y, float z);
    void setAmbientColor(unsigned int color);
    void setLightAbsorption(float absorption);
    void setLightScattering(float scattering);
    
    // Quality
    void setMarchSteps(int steps) { m_marchSteps = steps; }
    void setLightSampleSteps(int steps) { m_lightSampleSteps = steps; }
    void setDetailLevel(int level);
    
    // Animation
    void setWindVelocity(float x, float y, float z);
    void update(float deltaTime);
    
    // Toggles
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    bool isEnabled() const { return m_enabled; }

private:
    VolumetricClouds();
    VolumetricClouds(const VolumetricClouds&) = delete;
    VolumetricClouds& operator=(const VolumetricClouds&) = delete;

    void createNoiseTextures();
    void destroyNoiseTextures();
    void generateWorleyNoise();
    void generatePerlinNoise();

    int m_screenWidth;
    int m_screenHeight;
    
    unsigned int m_cloudTexture;
    unsigned int m_worleyNoise;
    unsigned int m_perlinNoise;
    unsigned int m_framebuffer;
    unsigned int m_shader;
    
    float m_coverage;
    float m_density;
    float m_scale;
    float m_minAltitude;
    float m_maxAltitude;
    
    float m_sunDirX;
    float m_sunDirY;
    float m_sunDirZ;
    unsigned int m_ambientColor;
    float m_lightAbsorption;
    float m_lightScattering;
    
    int m_marchSteps;
    int m_lightSampleSteps;
    int m_detailLevel;
    
    float m_windVelocityX;
    float m_windVelocityY;
    float m_windVelocityZ;
    float m_time;
    
    bool m_enabled;
};

} // namespace Engine
