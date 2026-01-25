#pragma once

#include <string>
#include <vector>
#include <functional>

namespace JJM {
namespace Graphics {

/**
 * Weather condition types
 */
enum class WeatherType {
    CLEAR,          // Clear sky
    CLOUDY,         // Overcast
    FOG,            // Foggy
    RAIN,           // Light rain
    HEAVY_RAIN,     // Heavy rain/storm
    SNOW,           // Snowing
    BLIZZARD,       // Heavy snow/blizzard
    SANDSTORM,      // Sandstorm (desert)
    THUNDER,        // Thunderstorm
    CUSTOM          // Custom weather
};

/**
 * Wind configuration
 */
struct Wind {
    float direction[3] = {1, 0, 0};     // Wind direction vector
    float speed = 5.0f;                 // Wind speed
    float gustStrength = 2.0f;          // Gust intensity
    float gustFrequency = 1.0f;         // Gusts per second
};

/**
 * Precipitation configuration
 */
struct Precipitation {
    float intensity = 0.0f;             // 0-1 (0 = none, 1 = heavy)
    float particleSize = 1.0f;          // Particle size multiplier
    float fallSpeed = 10.0f;            // Fall speed
    bool affectsVisibility = true;      // Reduces visibility
    float soundVolume = 0.5f;           // Ambient sound volume
};

/**
 * Atmosphere effects
 */
struct AtmosphereEffects {
    float fogDensity = 0.0f;            // 0-1
    float fogColor[3] = {0.7f, 0.7f, 0.7f};
    float lightingMultiplier = 1.0f;    // Overall lighting modifier
    float ambientSoundVolume = 0.0f;    // Wind/environment sounds
    float visibilityDistance = 1000.0f; // Maximum visibility in meters
};

/**
 * Complete weather state
 */
struct WeatherState {
    WeatherType type = WeatherType::CLEAR;
    Wind wind;
    Precipitation precipitation;
    AtmosphereEffects atmosphere;
    
    float duration = -1.0f;             // -1 = indefinite, else duration in seconds
    float elapsed = 0.0f;               // Time elapsed in current weather
    bool transitioning = false;
    float transitionProgress = 0.0f;
};

/**
 * Weather change event
 */
struct WeatherChangeEvent {
    WeatherType oldWeather;
    WeatherType newWeather;
    float transitionTime;
};

/**
 * Weather preset definition
 */
struct WeatherPreset {
    std::string name;
    WeatherType type;
    Wind wind;
    Precipitation precipitation;
    AtmosphereEffects atmosphere;
    float defaultDuration = -1.0f;      // Default duration when applied
};

/**
 * System for managing dynamic weather and atmospheric effects
 */
class WeatherSystem {
public:
    WeatherSystem();
    ~WeatherSystem();
    
    /**
     * Initialize the weather system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update weather effects and transitions
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Weather control
    
    /**
     * Set current weather
     * @param type Weather type
     * @param transitionTime Time to transition in seconds (0 = instant)
     * @param duration Weather duration in seconds (-1 = indefinite)
     */
    void setWeather(WeatherType type, float transitionTime = 2.0f, float duration = -1.0f);
    
    /**
     * Set weather from preset
     * @param presetName Name of preset
     * @param transitionTime Transition time in seconds
     */
    void setWeatherFromPreset(const std::string& presetName, float transitionTime = 2.0f);
    
    /**
     * Get current weather type
     * @return Current weather
     */
    WeatherType getCurrentWeather() const { return m_currentState.type; }
    
    /**
     * Clear weather (set to clear sky)
     * @param transitionTime Transition time in seconds
     */
    void clearWeather(float transitionTime = 2.0f);
    
    /**
     * Enable random weather changes
     * @param enabled True to enable
     * @param minDuration Minimum duration for each weather
     * @param maxDuration Maximum duration for each weather
     */
    void setRandomWeather(bool enabled, float minDuration = 300.0f, float maxDuration = 900.0f);
    
    /**
     * Check if transitioning between weather states
     * @return True if transitioning
     */
    bool isTransitioning() const { return m_currentState.transitioning; }
    
    // Wind control
    
    /**
     * Set wind configuration
     * @param wind Wind settings
     */
    void setWind(const Wind& wind);
    
    /**
     * Get current wind
     * @return Current wind configuration
     */
    const Wind& getWind() const { return m_currentState.wind; }
    
    /**
     * Set wind direction
     * @param direction Direction vector [x, y, z]
     */
    void setWindDirection(const float direction[3]);
    
    /**
     * Set wind speed
     * @param speed Wind speed
     */
    void setWindSpeed(float speed);
    
    // Precipitation control
    
    /**
     * Set precipitation configuration
     * @param precipitation Precipitation settings
     */
    void setPrecipitation(const Precipitation& precipitation);
    
    /**
     * Get current precipitation
     * @return Current precipitation configuration
     */
    const Precipitation& getPrecipitation() const { return m_currentState.precipitation; }
    
    /**
     * Set precipitation intensity
     * @param intensity Intensity (0-1)
     */
    void setPrecipitationIntensity(float intensity);
    
    // Atmosphere control
    
    /**
     * Set atmosphere effects
     * @param atmosphere Atmosphere settings
     */
    void setAtmosphere(const AtmosphereEffects& atmosphere);
    
    /**
     * Get current atmosphere
     * @return Current atmosphere configuration
     */
    const AtmosphereEffects& getAtmosphere() const { return m_currentState.atmosphere; }
    
    /**
     * Set fog density
     * @param density Fog density (0-1)
     */
    void setFogDensity(float density);
    
    /**
     * Set visibility distance
     * @param distance Visibility in meters
     */
    void setVisibilityDistance(float distance);
    
    // Presets
    
    /**
     * Register a weather preset
     * @param preset Preset to register
     */
    void registerPreset(const WeatherPreset& preset);
    
    /**
     * Load presets from file
     * @param filepath Path to preset file
     * @return Number of presets loaded
     */
    int loadPresets(const std::string& filepath);
    
    /**
     * Get preset by name
     * @param name Preset name
     * @return Pointer to preset, or nullptr if not found
     */
    const WeatherPreset* getPreset(const std::string& name) const;
    
    /**
     * Get all preset names
     * @return Vector of preset names
     */
    std::vector<std::string> getPresetNames() const;
    
    // Callbacks
    
    /**
     * Set callback for weather changes
     * @param callback Function called when weather changes
     */
    void setWeatherChangeCallback(std::function<void(const WeatherChangeEvent&)> callback);
    
    /**
     * Set callback for thunder strikes
     * @param callback Function called when thunder strikes (for visual/audio)
     */
    void setThunderCallback(std::function<void(const float[3])> callback);
    
    // Configuration
    
    /**
     * Enable or disable weather system
     * @param enabled True to enable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * Check if system is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Enable or disable weather effects (particles, sound, etc.)
     * @param enabled True to enable
     */
    void setEffectsEnabled(bool enabled) { m_effectsEnabled = enabled; }
    
    /**
     * Set intensity multiplier for all effects
     * @param multiplier Effect intensity (0-1)
     */
    void setGlobalIntensity(float multiplier) { m_globalIntensity = multiplier; }
    
    // State access
    
    /**
     * Get full weather state
     * @return Current state
     */
    const WeatherState& getState() const { return m_currentState; }
    
    /**
     * Save weather state to file
     * @param filepath Path to save file
     * @return True if saved successfully
     */
    bool saveState(const std::string& filepath);
    
    /**
     * Load weather state from file
     * @param filepath Path to load file
     * @return True if loaded successfully
     */
    bool loadState(const std::string& filepath);
    
    // Statistics
    
    /**
     * Statistics about weather system
     */
    struct Statistics {
        float timeInClear = 0.0f;
        float timeInRain = 0.0f;
        float timeInSnow = 0.0f;
        float timeInStorm = 0.0f;
        int weatherChanges = 0;
        int thunderStrikes = 0;
    };
    
    /**
     * Get weather statistics
     * @return Current statistics
     */
    Statistics getStatistics() const { return m_stats; }

private:
    WeatherState m_currentState;
    WeatherState m_targetState;
    WeatherState m_previousState;
    
    // Presets
    std::unordered_map<std::string, WeatherPreset> m_presets;
    
    // Random weather
    bool m_randomWeatherEnabled;
    float m_minWeatherDuration;
    float m_maxWeatherDuration;
    
    // Callbacks
    std::function<void(const WeatherChangeEvent&)> m_weatherChangeCallback;
    std::function<void(const float[3])> m_thunderCallback;
    
    // Settings
    bool m_enabled;
    bool m_effectsEnabled;
    float m_globalIntensity;
    
    // Thunder (for thunderstorm weather)
    float m_timeSinceLastThunder;
    float m_nextThunderTime;
    
    // Statistics
    Statistics m_stats;
    
    // Internal methods
    void transitionWeather(float deltaTime);
    void updateThunder(float deltaTime);
    void updateWindGusts(float deltaTime);
    void interpolateStates(const WeatherState& from, const WeatherState& to, float t);
    void initializePresets();
    void applyWeatherPreset(const WeatherPreset& preset);
    void selectRandomWeather();
    float randomFloat(float min, float max);
};

// Helper functions
namespace WeatherHelpers {
    /**
     * Get string name for weather type
     * @param type Weather type
     * @return String name
     */
    const char* getWeatherTypeName(WeatherType type);
    
    /**
     * Get default preset for a weather type
     * @param type Weather type
     * @return Weather preset
     */
    WeatherPreset getDefaultPreset(WeatherType type);
}

} // namespace Graphics
} // namespace JJM
