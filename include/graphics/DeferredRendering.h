#pragma once

#include <vector>
#include <string>

namespace Engine {

/**
 * @brief G-Buffer texture types
 */
enum class GBufferTexture {
    Albedo,         // RGB: albedo/diffuse color, A: unused
    Normal,         // RGB: world space normal, A: unused
    Position,       // RGB: world space position, A: depth
    MetallicRoughness, // R: metallic, G: roughness, B: AO, A: unused
    Emission,       // RGB: emission color, A: intensity
    Depth           // Depth buffer
};

/**
 * @brief Deferred rendering pass types
 */
enum class DeferredPass {
    Geometry,       // Render to G-Buffer
    Lighting,       // Full screen lighting pass
    Transparent,    // Forward rendering for transparent objects
    PostProcess     // Post-processing effects
};

/**
 * @brief Light types for deferred lighting
 */
enum class LightType {
    Directional,
    Point,
    Spot,
    Area
};

/**
 * @brief Light data for deferred shading
 */
struct DeferredLight {
    LightType type;
    float positionX, positionY, positionZ;
    float directionX, directionY, directionZ;
    float colorR, colorG, colorB;
    float intensity;
    float range;            // For point/spot lights
    float innerConeAngle;   // For spot lights
    float outerConeAngle;   // For spot lights
    bool castsShadows;
    
    DeferredLight()
        : type(LightType::Point)
        , positionX(0.0f), positionY(0.0f), positionZ(0.0f)
        , directionX(0.0f), directionY(-1.0f), directionZ(0.0f)
        , colorR(1.0f), colorG(1.0f), colorB(1.0f)
        , intensity(1.0f)
        , range(10.0f)
        , innerConeAngle(30.0f)
        , outerConeAngle(45.0f)
        , castsShadows(false)
    {}
};

/**
 * @brief G-Buffer configuration
 */
struct GBufferSettings {
    int width;
    int height;
    bool useHDR;            // Use HDR render targets
    bool useSSAO;           // Enable screen space ambient occlusion
    int ssaoSamples;        // SSAO sample count
    float ssaoRadius;       // SSAO sampling radius
    bool useSSR;            // Enable screen space reflections
    int msaaSamples;        // MSAA sample count (0 = disabled)
    
    GBufferSettings()
        : width(1920)
        , height(1080)
        , useHDR(true)
        , useSSAO(true)
        , ssaoSamples(16)
        , ssaoRadius(0.5f)
        , useSSR(false)
        , msaaSamples(0)
    {}
};

/**
 * @brief Statistics for deferred rendering
 */
struct DeferredStats {
    int geometryDrawCalls;
    int lightCount;
    int visibleLights;
    float geometryPassTime;     // ms
    float lightingPassTime;     // ms
    float totalFrameTime;       // ms
    int trianglesRendered;
    
    DeferredStats()
        : geometryDrawCalls(0)
        , lightCount(0)
        , visibleLights(0)
        , geometryPassTime(0.0f)
        , lightingPassTime(0.0f)
        , totalFrameTime(0.0f)
        , trianglesRendered(0)
    {}
};

/**
 * @brief Deferred rendering system
 */
class DeferredRenderer {
public:
    DeferredRenderer();
    ~DeferredRenderer();
    
    // Initialization
    void initialize(int width, int height);
    void shutdown();
    void resize(int width, int height);
    
    // Configuration
    void setSettings(const GBufferSettings& settings);
    const GBufferSettings& getSettings() const { return m_settings; }
    
    // G-Buffer management
    void createGBuffer();
    void destroyGBuffer();
    unsigned int getGBufferTexture(GBufferTexture type) const;
    unsigned int getFramebuffer() const { return m_gBufferFBO; }
    
    // Rendering passes
    void beginGeometryPass();
    void endGeometryPass();
    void beginLightingPass();
    void endLightingPass();
    
    // Light management
    int addLight(const DeferredLight& light);
    void removeLight(int lightId);
    void updateLight(int lightId, const DeferredLight& light);
    DeferredLight* getLight(int lightId);
    const std::vector<DeferredLight>& getLights() const { return m_lights; }
    void clearLights();
    
    // Light culling
    void cullLights(const float viewProj[16]);
    const std::vector<int>& getVisibleLights() const { return m_visibleLights; }
    
    // SSAO
    void enableSSAO(bool enable);
    void generateSSAOKernel();
    void generateSSAONoise();
    void renderSSAO();
    unsigned int getSSAOTexture() const { return m_ssaoTexture; }
    
    // Lighting
    void renderDirectionalLight(const DeferredLight& light);
    void renderPointLight(const DeferredLight& light);
    void renderSpotLight(const DeferredLight& light);
    
    // Debug
    void enableDebugVisualization(bool enable) { m_debugVisualization = enable; }
    void setDebugTexture(GBufferTexture texture) { m_debugTexture = texture; }
    void renderDebugView();
    
    // Stats
    const DeferredStats& getStats() const { return m_stats; }
    void resetStats();
    
private:
    void createGBufferTexture(GBufferTexture type);
    void setupGeometryShader();
    void setupLightingShader();
    void calculateLightBounds(const DeferredLight& light, float& radius) const;
    bool isLightVisible(const DeferredLight& light, const float viewProj[16]) const;
    
    GBufferSettings m_settings;
    
    // G-Buffer resources
    unsigned int m_gBufferFBO;
    unsigned int m_albedoTexture;
    unsigned int m_normalTexture;
    unsigned int m_positionTexture;
    unsigned int m_metallicRoughnessTexture;
    unsigned int m_emissionTexture;
    unsigned int m_depthTexture;
    
    // SSAO resources
    unsigned int m_ssaoFBO;
    unsigned int m_ssaoTexture;
    unsigned int m_ssaoBlurFBO;
    unsigned int m_ssaoBlurTexture;
    unsigned int m_ssaoNoiseTexture;
    std::vector<float> m_ssaoKernel;
    
    // Light data
    std::vector<DeferredLight> m_lights;
    std::vector<int> m_visibleLights;
    int m_nextLightId;
    
    // Shaders
    unsigned int m_geometryShader;
    unsigned int m_lightingShader;
    unsigned int m_directionalLightShader;
    unsigned int m_pointLightShader;
    unsigned int m_spotLightShader;
    unsigned int m_ssaoShader;
    unsigned int m_ssaoBlurShader;
    
    // Debug
    bool m_debugVisualization;
    GBufferTexture m_debugTexture;
    
    // Stats
    DeferredStats m_stats;
};

/**
 * @brief Global deferred rendering system
 */
class DeferredRenderingSystem {
public:
    static DeferredRenderingSystem& getInstance();
    
    void initialize(int width, int height);
    void shutdown();
    
    DeferredRenderer& getRenderer() { return m_renderer; }
    
    // Convenience methods
    int addLight(const DeferredLight& light) { return m_renderer.addLight(light); }
    void removeLight(int lightId) { m_renderer.removeLight(lightId); }
    
    // Enable/disable
    void enable(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    DeferredRenderingSystem() : m_enabled(true) {}
    DeferredRenderingSystem(const DeferredRenderingSystem&) = delete;
    DeferredRenderingSystem& operator=(const DeferredRenderingSystem&) = delete;
    
    DeferredRenderer m_renderer;
    bool m_enabled;
};

} // namespace Engine
