#pragma once

#include <string>
#include <vector>

namespace Engine {

/**
 * @brief Weather types
 */
enum class WeatherType {
    Clear,
    Cloudy,
    Overcast,
    Rain,
    HeavyRain,
    Thunderstorm,
    Snow,
    Blizzard,
    Fog,
    Mist
};

/**
 * @brief Precipitation data
 */
struct Precipitation {
    bool enabled;
    float intensity;        // 0-1 intensity
    float particleSize;     // Particle size
    float fallSpeed;        // Fall speed
    int particleCount;      // Number of particles
    float windInfluence;    // Wind effect on particles
    
    Precipitation()
        : enabled(false)
        , intensity(0.0f)
        , particleSize(1.0f)
        , fallSpeed(5.0f)
        , particleCount(1000)
        , windInfluence(0.5f)
    {}
};

/**
 * @brief Cloud configuration
 */
struct CloudConfig {
    bool enabled;
    float coverage;         // 0-1 cloud coverage
    float thickness;        // Cloud thickness
    float speed;           // Movement speed
    float altitude;        // Cloud altitude
    float density;         // Cloud density
    int layers;            // Number of cloud layers
    
    CloudConfig()
        : enabled(true)
        , coverage(0.5f)
        , thickness(10.0f)
        , speed(1.0f)
        , altitude(1000.0f)
        , density(0.8f)
        , layers(2)
    {}
};

/**
 * @brief Wind parameters
 */
struct Wind {
    float directionX;      // Wind direction X
    float directionY;      // Wind direction Y
    float directionZ;      // Wind direction Z
    float speed;           // Wind speed
    float turbulence;      // Turbulence amount
    float gustStrength;    // Gust strength
    float gustFrequency;   // Gust frequency
    
    Wind()
        : directionX(1.0f)
        , directionY(0.0f)
        , directionZ(0.0f)
        , speed(5.0f)
        , turbulence(0.3f)
        , gustStrength(0.5f)
        , gustFrequency(2.0f)
    {}
};

/**
 * @brief Fog settings
 */
struct FogSettings {
    bool enabled;
    float density;         // Fog density
    float startDistance;   // Fog start distance
    float endDistance;     // Fog end distance
    float height;          // Fog height
    float heightFalloff;   // Height falloff
    float colorR;          // Fog color RGB
    float colorG;
    float colorB;
    
    FogSettings()
        : enabled(false)
        , density(0.01f)
        , startDistance(10.0f)
        , endDistance(100.0f)
        , height(0.0f)
        , heightFalloff(0.1f)
        , colorR(0.5f)
        , colorG(0.5f)
        , colorB(0.5f)
    {}
};

/**
 * @brief Lightning strike data
 */
struct Lightning {
    bool active;
    float positionX;
    float positionY;
    float positionZ;
    float intensity;
    float duration;
    float timeRemaining;
    
    Lightning()
        : active(false)
        , positionX(0.0f)
        , positionY(0.0f)
        , positionZ(0.0f)
        , intensity(1.0f)
        , duration(0.2f)
        , timeRemaining(0.0f)
    {}
};

/**
 * @brief Weather state
 */
struct WeatherState {
    WeatherType type;
    float transition;      // Transition progress (0-1)
    float wetness;         // Surface wetness
    float temperature;     // Temperature in Celsius
    float humidity;        // Humidity percentage
    float visibility;      // Visibility distance
    
    Precipitation precipitation;
    CloudConfig clouds;
    Wind wind;
    FogSettings fog;
    std::vector<Lightning> lightning;
    
    WeatherState()
        : type(WeatherType::Clear)
        , transition(1.0f)
        , wetness(0.0f)
        , temperature(20.0f)
        , humidity(50.0f)
        , visibility(1000.0f)
    {}
};

/**
 * @brief Weather transition
 */
struct WeatherTransition {
    WeatherType from;
    WeatherType to;
    float duration;        // Transition duration in seconds
    float elapsed;         // Elapsed time
    bool active;
    
    WeatherTransition()
        : from(WeatherType::Clear)
        , to(WeatherType::Clear)
        , duration(10.0f)
        , elapsed(0.0f)
        , active(false)
    {}
};

/**
 * @brief Dynamic weather system
 * 
 * Manages realistic weather with:
 * - Weather type transitions
 * - Precipitation (rain, snow)
 * - Dynamic clouds
 * - Wind simulation
 * - Fog effects
 * - Lightning strikes
 */
class DynamicWeather {
public:
    DynamicWeather();
    ~DynamicWeather();
    
    // Weather control
    void setWeather(WeatherType type, float transitionTime = 10.0f);
    WeatherType getWeatherType() const { return m_currentState.type; }
    const WeatherState& getWeatherState() const { return m_currentState; }
    
    // Update
    void update(float deltaTime);
    
    // State queries
    bool isRaining() const;
    bool isSnowing() const;
    bool hasLightning() const;
    float getWetness() const { return m_currentState.wetness; }
    float getVisibility() const { return m_currentState.visibility; }
    
    // Configuration
    void setPrecipitation(const Precipitation& precip);
    void setClouds(const CloudConfig& clouds);
    void setWind(const Wind& wind);
    void setFog(const FogSettings& fog);
    
    // Lightning
    void triggerLightning(float x, float y, float z);
    void setLightningFrequency(float frequency);
    float getLightningFrequency() const { return m_lightningFrequency; }
    
    // Temperature and effects
    void setTemperature(float celsius);
    float getTemperature() const { return m_currentState.temperature; }
    void setHumidity(float percent);
    float getHumidity() const { return m_currentState.humidity; }
    
    // Rendering
    void renderPrecipitation();
    void renderClouds();
    void renderLightning();
    
    // Audio integration
    float getThunderVolume() const;
    float getRainVolume() const;
    float getWindVolume() const;
    
private:
    void updateTransition(float deltaTime);
    void updatePrecipitation(float deltaTime);
    void updateClouds(float deltaTime);
    void updateWind(float deltaTime);
    void updateFog(float deltaTime);
    void updateLightning(float deltaTime);
    void updateEffects(float deltaTime);
    
    WeatherState getWeatherPreset(WeatherType type) const;
    void interpolateStates(const WeatherState& from, const WeatherState& to, float t);
    
    WeatherState m_currentState;
    WeatherState m_targetState;
    WeatherTransition m_transition;
    
    // Lightning
    float m_lightningFrequency;
    float m_lightningTimer;
    
    // Timing
    float m_time;
    
    // Render resources
    unsigned int m_precipitationVAO;
    unsigned int m_precipitationVBO;
    unsigned int m_cloudShader;
    unsigned int m_precipShader;
    unsigned int m_lightningShader;
};

/**
 * @brief Weather presets
 */
struct WeatherPreset {
    std::string name;
    WeatherType type;
    WeatherState state;
    
    WeatherPreset(const std::string& n, WeatherType t)
        : name(n), type(t) {}
};

/**
 * @brief Global weather system
 */
class WeatherSystem {
public:
    static WeatherSystem& getInstance();
    
    void initialize();
    void shutdown();
    void update(float deltaTime);
    
    DynamicWeather& getWeather() { return m_weather; }
    
    // Presets
    void addPreset(const WeatherPreset& preset);
    WeatherPreset* getPreset(const std::string& name);
    void applyPreset(const std::string& name);
    
    // Time of day integration
    void setTimeOfDay(float hours);
    float getTimeOfDay() const { return m_timeOfDay; }
    
private:
    WeatherSystem() : m_timeOfDay(12.0f) {}
    
    DynamicWeather m_weather;
    std::vector<WeatherPreset> m_presets;
    float m_timeOfDay;
};

} // namespace Engine
