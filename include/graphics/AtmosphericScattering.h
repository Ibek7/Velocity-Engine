#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace Engine {

/**
 * @brief Atmospheric scattering types
 */
enum class ScatteringType {
    Rayleigh,       // Molecular scattering (blue sky)
    Mie,            // Aerosol scattering (haze, fog)
    Combined        // Both types
};

/**
 * @brief Time of day phases
 */
enum class TimeOfDay {
    Dawn,
    Morning,
    Noon,
    Afternoon,
    Dusk,
    Night
};

/**
 * @brief Weather conditions
 */
enum class WeatherType {
    Clear,
    Cloudy,
    Overcast,
    Rainy,
    Stormy,
    Foggy,
    Snowy
};

/**
 * @brief Atmosphere parameters
 */
struct AtmosphereParameters {
    // Planet properties
    float planetRadius;         // km
    float atmosphereRadius;     // km
    
    // Rayleigh scattering (molecules)
    float rayleighScaleHeight;  // km
    float rayleighR, rayleighG, rayleighB;  // Scattering coefficients
    
    // Mie scattering (aerosols)
    float mieScaleHeight;       // km
    float mieScattering;
    float mieAbsorption;
    float mieG;                 // Asymmetry factor (-1 to 1)
    
    // Ozone absorption
    float ozoneScaleHeight;     // km
    float ozoneAbsorption;
    
    // Sun
    float sunIntensity;
    float sunAngularSize;       // degrees
    
    AtmosphereParameters()
        : planetRadius(6371.0f)
        , atmosphereRadius(6471.0f)
        , rayleighScaleHeight(8.0f)
        , rayleighR(5.8e-6f), rayleighG(13.5e-6f), rayleighB(33.1e-6f)
        , mieScaleHeight(1.2f)
        , mieScattering(21e-6f)
        , mieAbsorption(4.4e-6f)
        , mieG(0.76f)
        , ozoneScaleHeight(25.0f)
        , ozoneAbsorption(0.65f)
        , sunIntensity(22.0f)
        , sunAngularSize(0.545f)
    {}
};

/**
 * @brief Sky dome configuration
 */
struct SkyDomeSettings {
    int resolution;             // Tessellation resolution
    float domeRadius;           // Sky dome radius
    bool dynamicTimeOfDay;      // Animate time of day
    float timeSpeed;            // Time progression speed
    bool renderClouds;          // Enable cloud layer
    bool renderStars;           // Enable star field
    int starCount;
    
    SkyDomeSettings()
        : resolution(64)
        , domeRadius(10000.0f)
        , dynamicTimeOfDay(true)
        , timeSpeed(1.0f)
        , renderClouds(true)
        , renderStars(true)
        , starCount(5000)
    {}
};

/**
 * @brief Cloud layer properties
 */
struct CloudLayer {
    float altitude;             // km above surface
    float thickness;            // km
    float coverage;             // 0-1
    float density;
    float speedX, speedY;       // Wind speed
    float scaleX, scaleY;       // Noise scale
    int octaves;
    
    CloudLayer()
        : altitude(2.0f)
        , thickness(1.0f)
        , coverage(0.5f)
        , density(0.8f)
        , speedX(0.01f), speedY(0.005f)
        , scaleX(1.0f), scaleY(1.0f)
        , octaves(4)
    {}
};

/**
 * @brief Celestial body (sun, moon)
 */
struct CelestialBody {
    std::string name;
    float azimuth;              // Horizontal angle (radians)
    float elevation;            // Vertical angle (radians)
    float angularSize;          // degrees
    float intensity;
    float colorR, colorG, colorB;
    bool castsShadows;
    unsigned int texture;
    
    CelestialBody()
        : azimuth(0.0f), elevation(0.0f)
        , angularSize(0.5f), intensity(1.0f)
        , colorR(1.0f), colorG(1.0f), colorB(1.0f)
        , castsShadows(true), texture(0)
    {}
};

/**
 * @brief Atmospheric rendering system
 */
class AtmosphericScattering {
public:
    AtmosphericScattering();
    ~AtmosphericScattering();
    
    // Initialization
    void initialize();
    void shutdown();
    
    // Configuration
    void setParameters(const AtmosphereParameters& params);
    const AtmosphereParameters& getParameters() const { return m_params; }
    void setSettings(const SkyDomeSettings& settings);
    const SkyDomeSettings& getSettings() const { return m_settings; }
    
    // Time of day
    void setTime(float hours);  // 0-24
    float getTime() const { return m_currentTime; }
    void setTimeOfDay(TimeOfDay tod);
    TimeOfDay getTimeOfDay() const;
    
    // Weather
    void setWeather(WeatherType weather);
    WeatherType getWeather() const { return m_weather; }
    void transitionWeather(WeatherType target, float duration);
    
    // Celestial bodies
    void updateSun();
    void updateMoon();
    const CelestialBody& getSun() const { return m_sun; }
    const CelestialBody& getMoon() const { return m_moon; }
    
    // Cloud layers
    void addCloudLayer(const CloudLayer& layer);
    void removeCloudLayer(int index);
    void updateClouds(float deltaTime);
    
    // Scattering calculations
    void calculateInscattering(float viewDirX, float viewDirY, float viewDirZ,
                               float& outR, float& outG, float& outB) const;
    void calculateTransmittance(float viewDirX, float viewDirY, float viewDirZ,
                                float distance, float& outR, float& outG, float& outB) const;
    float calculatePhaseRayleigh(float cosTheta) const;
    float calculatePhaseMie(float cosTheta, float g) const;
    
    // Ray-atmosphere intersection
    bool rayIntersectSphere(float rayX, float rayY, float rayZ,
                           float dirX, float dirY, float dirZ,
                           float radius, float& t) const;
    float atmosphericDepth(float rayX, float rayY, float rayZ,
                          float dirX, float dirY, float dirZ,
                          float scaleHeight) const;
    
    // Rendering
    void render(float cameraX, float cameraY, float cameraZ);
    void renderSkyDome();
    void renderClouds();
    void renderStars();
    void renderCelestialBodies();
    
    // Shader management
    void setupShaders();
    void updateUniforms();
    
    // Lookup tables (for optimization)
    void generateTransmittanceLUT();
    void generateMultiScatteringLUT();
    void generateSkyViewLUT();
    
    // Update
    void update(float deltaTime);
    
private:
    void calculateSunPosition(float time, float& azimuth, float& elevation) const;
    void calculateMoonPosition(float time, float& azimuth, float& elevation) const;
    void interpolateWeather(float t);
    void createSkyDomeMesh();
    void createCloudMesh();
    
    AtmosphereParameters m_params;
    SkyDomeSettings m_settings;
    
    float m_currentTime;        // 0-24 hours
    WeatherType m_weather;
    WeatherType m_targetWeather;
    float m_weatherTransitionTime;
    float m_weatherTransitionDuration;
    
    CelestialBody m_sun;
    CelestialBody m_moon;
    std::vector<CloudLayer> m_cloudLayers;
    
    // Sky dome mesh
    unsigned int m_skyVAO;
    unsigned int m_skyVBO;
    unsigned int m_skyIBO;
    int m_skyIndexCount;
    
    // Lookup textures
    unsigned int m_transmittanceLUT;
    unsigned int m_multiScatterLUT;
    unsigned int m_skyViewLUT;
    
    // Shaders
    unsigned int m_skyShader;
    unsigned int m_cloudShader;
    unsigned int m_starShader;
    unsigned int m_celestialShader;
};

/**
 * @brief Global atmospheric system
 */
class AtmosphericSystem {
public:
    static AtmosphericSystem& getInstance();
    
    void initialize();
    void shutdown();
    
    AtmosphericScattering& getScattering() { return m_scattering; }
    
    // Convenience methods
    void setTime(float hours) { m_scattering.setTime(hours); }
    void setWeather(WeatherType weather) { m_scattering.setWeather(weather); }
    void update(float deltaTime) { m_scattering.update(deltaTime); }
    void render(float x, float y, float z) { m_scattering.render(x, y, z); }
    
    // Enable/disable
    void enable(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    AtmosphericSystem() : m_enabled(true) {}
    AtmosphericSystem(const AtmosphericSystem&) = delete;
    AtmosphericSystem& operator=(const AtmosphericSystem&) = delete;
    
    AtmosphericScattering m_scattering;
    bool m_enabled;
};

} // namespace Engine
