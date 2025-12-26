#pragma once

#include <vector>
#include <string>

namespace Engine {

/**
 * @brief Volumetric light types
 */
enum class VolumetricLightType {
    Spotlight,
    Directional,
    Point,
    Area
};

/**
 * @brief Volume rendering quality
 */
enum class VolumeQuality {
    Low,        // 32 samples
    Medium,     // 64 samples
    High,       // 128 samples
    Ultra       // 256 samples
};

/**
 * @brief Volumetric light source
 */
struct VolumetricLight {
    VolumetricLightType type;
    float positionX, positionY, positionZ;
    float directionX, directionY, directionZ;
    float colorR, colorG, colorB;
    float intensity;
    float range;
    float coneAngle;            // For spotlights
    float scattering;           // Scattering coefficient
    bool castShadows;
    bool enabled;
    
    VolumetricLight()
        : type(VolumetricLightType::Spotlight)
        , positionX(0.0f), positionY(0.0f), positionZ(0.0f)
        , directionX(0.0f), directionY(-1.0f), directionZ(0.0f)
        , colorR(1.0f), colorG(1.0f), colorB(1.0f)
        , intensity(1.0f)
        , range(10.0f)
        , coneAngle(45.0f)
        , scattering(0.5f)
        , castShadows(true)
        , enabled(true)
    {}
};

/**
 * @brief Volume fog/atmosphere properties
 */
struct VolumeFog {
    float density;              // Base fog density
    float heightFalloff;        // Exponential height falloff
    float baseHeight;           // Height where fog starts
    float scatteringR, scatteringG, scatteringB;
    float absorptionR, absorptionG, absorptionB;
    float anisotropy;           // Phase function g parameter
    bool enabled;
    
    VolumeFog()
        : density(0.01f)
        , heightFalloff(0.1f)
        , baseHeight(0.0f)
        , scatteringR(1.0f), scatteringG(1.0f), scatteringB(1.0f)
        , absorptionR(0.1f), absorptionG(0.1f), absorptionB(0.1f)
        , anisotropy(0.3f)
        , enabled(false)
    {}
};

/**
 * @brief Volumetric rendering settings
 */
struct VolumetricSettings {
    VolumeQuality quality;
    int volumeWidth;            // Volume texture resolution
    int volumeHeight;
    int volumeDepth;
    float nearPlane;
    float farPlane;
    int rayMarchSteps;
    float ditherStrength;       // Temporal dithering
    bool useTemporalFiltering;
    float temporalBlend;
    
    VolumetricSettings()
        : quality(VolumeQuality::Medium)
        , volumeWidth(160)
        , volumeHeight(90)
        , volumeDepth(64)
        , nearPlane(0.1f)
        , farPlane(100.0f)
        , rayMarchSteps(64)
        , ditherStrength(1.0f)
        , useTemporalFiltering(true)
        , temporalBlend(0.9f)
    {}
};

/**
 * @brief Volumetric statistics
 */
struct VolumetricStats {
    int activeLights;
    int raymarchSamples;
    float renderTime;           // ms
    int volumeResolutionX;
    int volumeResolutionY;
    int volumeResolutionZ;
    
    VolumetricStats()
        : activeLights(0)
        , raymarchSamples(0)
        , renderTime(0.0f)
        , volumeResolutionX(0)
        , volumeResolutionY(0)
        , volumeResolutionZ(0)
    {}
};

/**
 * @brief Volumetric lighting system
 */
class VolumetricLighting {
public:
    VolumetricLighting();
    ~VolumetricLighting();
    
    // Initialization
    void initialize(int screenWidth, int screenHeight);
    void shutdown();
    void resize(int screenWidth, int screenHeight);
    
    // Configuration
    void setSettings(const VolumetricSettings& settings);
    const VolumetricSettings& getSettings() const { return m_settings; }
    void setQuality(VolumeQuality quality);
    
    // Light management
    int addLight(const VolumetricLight& light);
    void removeLight(int lightId);
    void updateLight(int lightId, const VolumetricLight& light);
    VolumetricLight* getLight(int lightId);
    void clearLights();
    
    // Fog
    void setFog(const VolumeFog& fog);
    const VolumeFog& getFog() const { return m_fog; }
    void enableFog(bool enable);
    
    // Rendering
    void render(float cameraX, float cameraY, float cameraZ,
                const float viewMatrix[16], const float projMatrix[16]);
    void renderVolumeTexture();
    void rayMarch();
    void applyVolume(unsigned int sceneTexture, unsigned int depthTexture);
    
    // Volume texture access
    unsigned int getVolumeTexture() const { return m_volumeTexture; }
    unsigned int getIntegratedScattering() const { return m_integratedScattering; }
    
    // Temporal filtering
    void updateTemporalHistory();
    void blendWithHistory();
    
    // Phase functions
    float calculatePhaseFunction(float cosTheta, float g) const;
    float calculatePhaseHG(float cosTheta, float g) const;
    float calculatePhaseSchlick(float cosTheta, float g) const;
    
    // Density functions
    float sampleDensity(float x, float y, float z) const;
    float calculateFogDensity(float height) const;
    
    // Light scattering
    void scatterLight(const VolumetricLight& light, float x, float y, float z,
                      float& outR, float& outG, float& outB) const;
    float calculateAttenuation(float distance, float range) const;
    
    // Shadow sampling
    float sampleShadowMap(const VolumetricLight& light, float x, float y, float z) const;
    
    // Statistics
    const VolumetricStats& getStats() const { return m_stats; }
    void resetStats();
    
private:
    void createVolumeTextures();
    void destroyVolumeTextures();
    void setupShaders();
    void updateUniforms();
    void generateFrustumCorners(const float invProjView[16]);
    
    VolumetricSettings m_settings;
    VolumeFog m_fog;
    std::vector<VolumetricLight> m_lights;
    
    // Volume textures
    unsigned int m_volumeTexture;
    unsigned int m_integratedScattering;
    unsigned int m_historyTexture;
    unsigned int m_volumeFBO;
    
    // Shaders
    unsigned int m_volumeShader;
    unsigned int m_raymarchShader;
    unsigned int m_compositeShader;
    unsigned int m_temporalShader;
    
    // Camera data
    float m_cameraX, m_cameraY, m_cameraZ;
    float m_viewMatrix[16];
    float m_projMatrix[16];
    
    // Screen size
    int m_screenWidth;
    int m_screenHeight;
    
    // Temporal data
    int m_frameIndex;
    
    VolumetricStats m_stats;
};

/**
 * @brief God rays / light shafts effect
 */
class GodRaysEffect {
public:
    GodRaysEffect();
    ~GodRaysEffect();
    
    void initialize(int width, int height);
    void shutdown();
    
    void render(unsigned int sceneTexture, unsigned int depthTexture,
                float sunScreenX, float sunScreenY);
    
    void setIntensity(float intensity) { m_intensity = intensity; }
    void setDecay(float decay) { m_decay = decay; }
    void setExposure(float exposure) { m_exposure = exposure; }
    void setNumSamples(int samples) { m_numSamples = samples; }
    
    unsigned int getOutputTexture() const { return m_outputTexture; }
    
private:
    void createRenderTargets();
    void radialBlur(float centerX, float centerY);
    
    int m_width, m_height;
    float m_intensity;
    float m_decay;
    float m_exposure;
    int m_numSamples;
    
    unsigned int m_occlusionFBO;
    unsigned int m_occlusionTexture;
    unsigned int m_blurFBO;
    unsigned int m_outputTexture;
    unsigned int m_shader;
};

/**
 * @brief Global volumetric lighting system
 */
class VolumetricLightingSystem {
public:
    static VolumetricLightingSystem& getInstance();
    
    void initialize(int screenWidth, int screenHeight);
    void shutdown();
    
    VolumetricLighting& getVolumetric() { return m_volumetric; }
    GodRaysEffect& getGodRays() { return m_godRays; }
    
    // Convenience methods
    int addLight(const VolumetricLight& light) { return m_volumetric.addLight(light); }
    void setFog(const VolumeFog& fog) { m_volumetric.setFog(fog); }
    
    void render(float cameraX, float cameraY, float cameraZ,
                const float viewMatrix[16], const float projMatrix[16]) {
        m_volumetric.render(cameraX, cameraY, cameraZ, viewMatrix, projMatrix);
    }
    
    // Enable/disable
    void enable(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    VolumetricLightingSystem() : m_enabled(true) {}
    VolumetricLightingSystem(const VolumetricLightingSystem&) = delete;
    VolumetricLightingSystem& operator=(const VolumetricLightingSystem&) = delete;
    
    VolumetricLighting m_volumetric;
    GodRaysEffect m_godRays;
    bool m_enabled;
};

} // namespace Engine
