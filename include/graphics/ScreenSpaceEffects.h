#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// Screen space reflections (SSR)
namespace Engine {

class ScreenSpaceReflections {
public:
    static ScreenSpaceReflections& getInstance();

    // Setup
    void initialize(int screenWidth, int screenHeight);
    void shutdown();
    
    // Rendering
    void render(unsigned int sceneTexture, unsigned int normalTexture, 
               unsigned int depthTexture);
    unsigned int getReflectionTexture() const { return m_reflectionTexture; }
    
    // Parameters
    void setRayStep(float step) { m_rayStep = step; }
    void setMaxSteps(int steps) { m_maxSteps = steps; }
    void setMaxDistance(float distance) { m_maxDistance = distance; }
    void setThickness(float thickness) { m_thickness = thickness; }
    void setFalloff(float falloff) { m_falloff = falloff; }
    
    // Quality
    void setBinarySearchSteps(int steps);
    void setJitterAmount(float amount);
    void enableTemporalFiltering(bool enable) { m_temporalFiltering = enable; }
    
    // Toggles
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    bool isEnabled() const { return m_enabled; }

private:
    ScreenSpaceReflections();
    ScreenSpaceReflections(const ScreenSpaceReflections&) = delete;
    ScreenSpaceReflections& operator=(const ScreenSpaceReflections&) = delete;

    void createRenderTargets();
    void destroyRenderTargets();
    void rayMarch();
    void applyBlur();

    int m_screenWidth;
    int m_screenHeight;
    
    unsigned int m_reflectionTexture;
    unsigned int m_framebuffer;
    unsigned int m_shader;
    
    float m_rayStep;
    int m_maxSteps;
    float m_maxDistance;
    float m_thickness;
    float m_falloff;
    int m_binarySearchSteps;
    float m_jitterAmount;
    bool m_temporalFiltering;
    bool m_enabled;
};

// Screen space ambient occlusion (SSAO)
class ScreenSpaceAO {
public:
    static ScreenSpaceAO& getInstance();

    // Setup
    void initialize(int screenWidth, int screenHeight);
    void shutdown();
    
    // Rendering
    void render(unsigned int normalTexture, unsigned int depthTexture);
    unsigned int getAOTexture() const { return m_aoTexture; }
    
    // Parameters
    void setRadius(float radius) { m_radius = radius; }
    void setBias(float bias) { m_bias = bias; }
    void setSampleCount(int count);
    void setPower(float power) { m_power = power; }
    
    // Quality
    void setBlurSize(int size) { m_blurSize = size; }
    void enableNoise(bool enable) { m_noiseEnabled = enable; }
    
    // Toggles
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    bool isEnabled() const { return m_enabled; }

private:
    ScreenSpaceAO();
    ScreenSpaceAO(const ScreenSpaceAO&) = delete;
    ScreenSpaceAO& operator=(const ScreenSpaceAO&) = delete;

    void createRenderTargets();
    void destroyRenderTargets();
    void generateSampleKernel();
    void generateNoiseTexture();
    void renderAO();
    void blurAO();

    int m_screenWidth;
    int m_screenHeight;
    
    unsigned int m_aoTexture;
    unsigned int m_blurTexture;
    unsigned int m_framebuffer;
    unsigned int m_shader;
    unsigned int m_noiseTexture;
    
    float m_radius;
    float m_bias;
    float m_power;
    int m_sampleCount;
    int m_blurSize;
    bool m_noiseEnabled;
    bool m_enabled;
    
    std::vector<float> m_sampleKernel;
};

} // namespace Engine
