#include "graphics/WeatherSystem.h"
#include <cmath>
#include <random>
#include <algorithm>
#include <fstream>
#include <unordered_map>

namespace JJM {
namespace Graphics {

namespace {
    // Random number generator
    std::random_device g_rd;
    std::mt19937 g_gen(g_rd());
}

// Constructor
WeatherSystem::WeatherSystem()
    : m_randomWeatherEnabled(false)
    , m_minWeatherDuration(300.0f)
    , m_maxWeatherDuration(900.0f)
    , m_enabled(true)
    , m_effectsEnabled(true)
    , m_globalIntensity(1.0f)
    , m_timeSinceLastThunder(0.0f)
    , m_nextThunderTime(5.0f)
{
    m_currentState.type = WeatherType::CLEAR;
    m_targetState = m_currentState;
    m_previousState = m_currentState;
}

// Destructor
WeatherSystem::~WeatherSystem() {
    shutdown();
}

// Initialize
void WeatherSystem::initialize() {
    // Initialize default presets
    initializePresets();
    
    // Reset statistics
    m_stats = Statistics();
}

// Shutdown
void WeatherSystem::shutdown() {
    m_presets.clear();
    m_weatherChangeCallback = nullptr;
    m_thunderCallback = nullptr;
}

// Update
void WeatherSystem::update(float deltaTime) {
    if (!m_enabled) return;
    
    // Update transition
    if (m_currentState.transitioning) {
        transitionWeather(deltaTime);
    }
    
    // Update wind gusts
    updateWindGusts(deltaTime);
    
    // Update thunder (if stormy)
    if (m_currentState.type == WeatherType::THUNDER) {
        updateThunder(deltaTime);
    }
    
    // Update duration tracking
    if (m_currentState.duration > 0.0f) {
        m_currentState.elapsed += deltaTime;
        
        // Duration expired, select new weather if random enabled
        if (m_currentState.elapsed >= m_currentState.duration) {
            if (m_randomWeatherEnabled) {
                selectRandomWeather();
            } else {
                // Just clear the duration
                m_currentState.duration = -1.0f;
            }
        }
    }
    
    // Update statistics
    switch (m_currentState.type) {
        case WeatherType::CLEAR:
        case WeatherType::CLOUDY:
            m_stats.timeInClear += deltaTime;
            break;
        case WeatherType::RAIN:
        case WeatherType::HEAVY_RAIN:
            m_stats.timeInRain += deltaTime;
            break;
        case WeatherType::SNOW:
        case WeatherType::BLIZZARD:
            m_stats.timeInSnow += deltaTime;
            break;
        case WeatherType::THUNDER:
        case WeatherType::SANDSTORM:
            m_stats.timeInStorm += deltaTime;
            break;
        default:
            break;
    }
}

// Set weather
void WeatherSystem::setWeather(WeatherType type, float transitionTime, float duration) {
    if (!m_enabled) return;
    
    WeatherType oldWeather = m_currentState.type;
    
    // Get default preset for this weather type
    WeatherPreset preset = WeatherHelpers::getDefaultPreset(type);
    preset.defaultDuration = duration;
    
    // Set target state
    m_targetState.type = type;
    m_targetState.wind = preset.wind;
    m_targetState.precipitation = preset.precipitation;
    m_targetState.atmosphere = preset.atmosphere;
    m_targetState.duration = duration;
    m_targetState.elapsed = 0.0f;
    
    if (transitionTime > 0.0f) {
        m_currentState.transitioning = true;
        m_currentState.transitionProgress = 0.0f;
        m_previousState = m_currentState;
        
        // Calculate transition speed
        m_currentState.duration = transitionTime;
    } else {
        // Instant change
        m_currentState = m_targetState;
        m_currentState.transitioning = false;
        m_currentState.duration = duration;
    }
    
    // Statistics
    m_stats.weatherChanges++;
    
    // Callback
    if (m_weatherChangeCallback && oldWeather != type) {
        WeatherChangeEvent event;
        event.oldWeather = oldWeather;
        event.newWeather = type;
        event.transitionTime = transitionTime;
        m_weatherChangeCallback(event);
    }
}

// Set weather from preset
void WeatherSystem::setWeatherFromPreset(const std::string& presetName, float transitionTime) {
    const WeatherPreset* preset = getPreset(presetName);
    if (!preset) return;
    
    applyWeatherPreset(*preset);
    
    if (transitionTime > 0.0f) {
        m_currentState.transitioning = true;
        m_currentState.transitionProgress = 0.0f;
        m_previousState = m_currentState;
        m_currentState.duration = transitionTime;
    }
}

// Clear weather
void WeatherSystem::clearWeather(float transitionTime) {
    setWeather(WeatherType::CLEAR, transitionTime, -1.0f);
}

// Set random weather
void WeatherSystem::setRandomWeather(bool enabled, float minDuration, float maxDuration) {
    m_randomWeatherEnabled = enabled;
    m_minWeatherDuration = minDuration;
    m_maxWeatherDuration = maxDuration;
    
    if (enabled) {
        selectRandomWeather();
    }
}

// Set wind
void WeatherSystem::setWind(const Wind& wind) {
    m_currentState.wind = wind;
}

// Set wind direction
void WeatherSystem::setWindDirection(const float direction[3]) {
    m_currentState.wind.direction[0] = direction[0];
    m_currentState.wind.direction[1] = direction[1];
    m_currentState.wind.direction[2] = direction[2];
    
    // Normalize
    float length = std::sqrt(
        direction[0] * direction[0] +
        direction[1] * direction[1] +
        direction[2] * direction[2]
    );
    if (length > 0.0f) {
        m_currentState.wind.direction[0] /= length;
        m_currentState.wind.direction[1] /= length;
        m_currentState.wind.direction[2] /= length;
    }
}

// Set wind speed
void WeatherSystem::setWindSpeed(float speed) {
    m_currentState.wind.speed = std::max(0.0f, speed);
}

// Set precipitation
void WeatherSystem::setPrecipitation(const Precipitation& precipitation) {
    m_currentState.precipitation = precipitation;
}

// Set precipitation intensity
void WeatherSystem::setPrecipitationIntensity(float intensity) {
    m_currentState.precipitation.intensity = std::clamp(intensity, 0.0f, 1.0f);
}

// Set atmosphere
void WeatherSystem::setAtmosphere(const AtmosphereEffects& atmosphere) {
    m_currentState.atmosphere = atmosphere;
}

// Set fog density
void WeatherSystem::setFogDensity(float density) {
    m_currentState.atmosphere.fogDensity = std::clamp(density, 0.0f, 1.0f);
}

// Set visibility distance
void WeatherSystem::setVisibilityDistance(float distance) {
    m_currentState.atmosphere.visibilityDistance = std::max(0.0f, distance);
}

// Register preset
void WeatherSystem::registerPreset(const WeatherPreset& preset) {
    m_presets[preset.name] = preset;
}

// Load presets from file
int WeatherSystem::loadPresets(const std::string& filepath) {
    // TODO: Implement file loading
    // For now, just return 0
    return 0;
}

// Get preset
const WeatherPreset* WeatherSystem::getPreset(const std::string& name) const {
    auto it = m_presets.find(name);
    if (it != m_presets.end()) {
        return &it->second;
    }
    return nullptr;
}

// Get preset names
std::vector<std::string> WeatherSystem::getPresetNames() const {
    std::vector<std::string> names;
    names.reserve(m_presets.size());
    
    for (const auto& pair : m_presets) {
        names.push_back(pair.first);
    }
    
    return names;
}

// Set weather change callback
void WeatherSystem::setWeatherChangeCallback(std::function<void(const WeatherChangeEvent&)> callback) {
    m_weatherChangeCallback = callback;
}

// Set thunder callback
void WeatherSystem::setThunderCallback(std::function<void(const float[3])> callback) {
    m_thunderCallback = callback;
}

// Save state
bool WeatherSystem::saveState(const std::string& filepath) {
    // TODO: Implement state saving
    return false;
}

// Load state
bool WeatherSystem::loadState(const std::string& filepath) {
    // TODO: Implement state loading
    return false;
}

// Transition weather
void WeatherSystem::transitionWeather(float deltaTime) {
    m_currentState.transitionProgress += deltaTime / m_currentState.duration;
    
    if (m_currentState.transitionProgress >= 1.0f) {
        // Transition complete
        m_currentState = m_targetState;
        m_currentState.transitioning = false;
        m_currentState.transitionProgress = 1.0f;
    } else {
        // Interpolate between previous and target
        interpolateStates(m_previousState, m_targetState, m_currentState.transitionProgress);
    }
}

// Update thunder
void WeatherSystem::updateThunder(float deltaTime) {
    m_timeSinceLastThunder += deltaTime;
    
    if (m_timeSinceLastThunder >= m_nextThunderTime) {
        // Thunder strike!
        m_timeSinceLastThunder = 0.0f;
        m_nextThunderTime = randomFloat(3.0f, 15.0f);
        
        m_stats.thunderStrikes++;
        
        // Call callback with random position (simplified - just above origin)
        if (m_thunderCallback) {
            float position[3] = {
                randomFloat(-500.0f, 500.0f),
                randomFloat(100.0f, 300.0f),
                randomFloat(-500.0f, 500.0f)
            };
            m_thunderCallback(position);
        }
    }
}

// Update wind gusts
void WeatherSystem::updateWindGusts(float deltaTime) {
    // Simple periodic gust simulation
    // Real implementation would use noise functions
    float gustPhase = std::sin(m_currentState.elapsed * m_currentState.wind.gustFrequency * 3.14159f);
    float currentGust = m_currentState.wind.gustStrength * gustPhase;
    
    // Wind speed varies with gusts
    // (This would be queried by rendering/physics systems)
}

// Interpolate states
void WeatherSystem::interpolateStates(const WeatherState& from, const WeatherState& to, float t) {
    // Lerp wind
    m_currentState.wind.speed = from.wind.speed + (to.wind.speed - from.wind.speed) * t;
    m_currentState.wind.gustStrength = from.wind.gustStrength + (to.wind.gustStrength - from.wind.gustStrength) * t;
    
    // Lerp wind direction
    for (int i = 0; i < 3; i++) {
        m_currentState.wind.direction[i] = from.wind.direction[i] + (to.wind.direction[i] - from.wind.direction[i]) * t;
    }
    
    // Lerp precipitation
    m_currentState.precipitation.intensity = from.precipitation.intensity + (to.precipitation.intensity - from.precipitation.intensity) * t;
    m_currentState.precipitation.fallSpeed = from.precipitation.fallSpeed + (to.precipitation.fallSpeed - from.precipitation.fallSpeed) * t;
    m_currentState.precipitation.soundVolume = from.precipitation.soundVolume + (to.precipitation.soundVolume - from.precipitation.soundVolume) * t;
    
    // Lerp atmosphere
    m_currentState.atmosphere.fogDensity = from.atmosphere.fogDensity + (to.atmosphere.fogDensity - from.atmosphere.fogDensity) * t;
    m_currentState.atmosphere.lightingMultiplier = from.atmosphere.lightingMultiplier + (to.atmosphere.lightingMultiplier - from.atmosphere.lightingMultiplier) * t;
    m_currentState.atmosphere.visibilityDistance = from.atmosphere.visibilityDistance + (to.atmosphere.visibilityDistance - from.atmosphere.visibilityDistance) * t;
    m_currentState.atmosphere.ambientSoundVolume = from.atmosphere.ambientSoundVolume + (to.atmosphere.ambientSoundVolume - from.atmosphere.ambientSoundVolume) * t;
    
    // Lerp fog color
    for (int i = 0; i < 3; i++) {
        m_currentState.atmosphere.fogColor[i] = from.atmosphere.fogColor[i] + (to.atmosphere.fogColor[i] - from.atmosphere.fogColor[i]) * t;
    }
}

// Initialize presets
void WeatherSystem::initializePresets() {
    // Clear preset
    {
        WeatherPreset preset;
        preset.name = "clear";
        preset.type = WeatherType::CLEAR;
        preset.wind.speed = 2.0f;
        preset.wind.gustStrength = 0.5f;
        preset.atmosphere.fogDensity = 0.0f;
        preset.atmosphere.lightingMultiplier = 1.0f;
        preset.atmosphere.visibilityDistance = 10000.0f;
        registerPreset(preset);
    }
    
    // Cloudy preset
    {
        WeatherPreset preset;
        preset.name = "cloudy";
        preset.type = WeatherType::CLOUDY;
        preset.wind.speed = 5.0f;
        preset.wind.gustStrength = 2.0f;
        preset.atmosphere.fogDensity = 0.1f;
        preset.atmosphere.lightingMultiplier = 0.7f;
        preset.atmosphere.visibilityDistance = 5000.0f;
        registerPreset(preset);
    }
    
    // Rain preset
    {
        WeatherPreset preset;
        preset.name = "rain";
        preset.type = WeatherType::RAIN;
        preset.wind.speed = 8.0f;
        preset.wind.gustStrength = 3.0f;
        preset.precipitation.intensity = 0.5f;
        preset.precipitation.fallSpeed = 15.0f;
        preset.precipitation.soundVolume = 0.4f;
        preset.atmosphere.fogDensity = 0.2f;
        preset.atmosphere.lightingMultiplier = 0.6f;
        preset.atmosphere.visibilityDistance = 2000.0f;
        registerPreset(preset);
    }
    
    // Heavy rain preset
    {
        WeatherPreset preset;
        preset.name = "heavy_rain";
        preset.type = WeatherType::HEAVY_RAIN;
        preset.wind.speed = 15.0f;
        preset.wind.gustStrength = 8.0f;
        preset.precipitation.intensity = 1.0f;
        preset.precipitation.fallSpeed = 25.0f;
        preset.precipitation.soundVolume = 0.8f;
        preset.atmosphere.fogDensity = 0.4f;
        preset.atmosphere.lightingMultiplier = 0.4f;
        preset.atmosphere.visibilityDistance = 500.0f;
        registerPreset(preset);
    }
    
    // Snow preset
    {
        WeatherPreset preset;
        preset.name = "snow";
        preset.type = WeatherType::SNOW;
        preset.wind.speed = 3.0f;
        preset.wind.gustStrength = 1.0f;
        preset.precipitation.intensity = 0.6f;
        preset.precipitation.fallSpeed = 2.0f;
        preset.precipitation.soundVolume = 0.1f;
        preset.atmosphere.fogDensity = 0.3f;
        preset.atmosphere.fogColor[0] = 0.9f;
        preset.atmosphere.fogColor[1] = 0.9f;
        preset.atmosphere.fogColor[2] = 0.95f;
        preset.atmosphere.lightingMultiplier = 0.8f;
        preset.atmosphere.visibilityDistance = 1000.0f;
        registerPreset(preset);
    }
    
    // Thunderstorm preset
    {
        WeatherPreset preset;
        preset.name = "thunder";
        preset.type = WeatherType::THUNDER;
        preset.wind.speed = 20.0f;
        preset.wind.gustStrength = 12.0f;
        preset.precipitation.intensity = 0.9f;
        preset.precipitation.fallSpeed = 30.0f;
        preset.precipitation.soundVolume = 0.7f;
        preset.atmosphere.fogDensity = 0.5f;
        preset.atmosphere.lightingMultiplier = 0.3f;
        preset.atmosphere.visibilityDistance = 300.0f;
        preset.atmosphere.ambientSoundVolume = 0.8f;
        registerPreset(preset);
    }
    
    // Fog preset
    {
        WeatherPreset preset;
        preset.name = "fog";
        preset.type = WeatherType::FOG;
        preset.wind.speed = 1.0f;
        preset.wind.gustStrength = 0.2f;
        preset.atmosphere.fogDensity = 0.8f;
        preset.atmosphere.lightingMultiplier = 0.5f;
        preset.atmosphere.visibilityDistance = 100.0f;
        registerPreset(preset);
    }
    
    // Sandstorm preset
    {
        WeatherPreset preset;
        preset.name = "sandstorm";
        preset.type = WeatherType::SANDSTORM;
        preset.wind.speed = 25.0f;
        preset.wind.gustStrength = 15.0f;
        preset.precipitation.intensity = 0.7f;
        preset.precipitation.fallSpeed = 5.0f;
        preset.atmosphere.fogDensity = 0.6f;
        preset.atmosphere.fogColor[0] = 0.8f;
        preset.atmosphere.fogColor[1] = 0.7f;
        preset.atmosphere.fogColor[2] = 0.5f;
        preset.atmosphere.lightingMultiplier = 0.4f;
        preset.atmosphere.visibilityDistance = 200.0f;
        preset.atmosphere.ambientSoundVolume = 0.9f;
        registerPreset(preset);
    }
}

// Apply weather preset
void WeatherSystem::applyWeatherPreset(const WeatherPreset& preset) {
    m_targetState.type = preset.type;
    m_targetState.wind = preset.wind;
    m_targetState.precipitation = preset.precipitation;
    m_targetState.atmosphere = preset.atmosphere;
    m_targetState.duration = preset.defaultDuration;
    m_targetState.elapsed = 0.0f;
}

// Select random weather
void WeatherSystem::selectRandomWeather() {
    // Get all non-custom weather types
    std::vector<WeatherType> types = {
        WeatherType::CLEAR,
        WeatherType::CLOUDY,
        WeatherType::FOG,
        WeatherType::RAIN,
        WeatherType::HEAVY_RAIN,
        WeatherType::SNOW,
        WeatherType::BLIZZARD,
        WeatherType::SANDSTORM,
        WeatherType::THUNDER
    };
    
    // Pick random
    std::uniform_int_distribution<> dist(0, types.size() - 1);
    WeatherType newWeather = types[dist(g_gen)];
    
    // Random duration
    float duration = randomFloat(m_minWeatherDuration, m_maxWeatherDuration);
    
    // Set with transition
    setWeather(newWeather, 5.0f, duration);
}

// Random float
float WeatherSystem::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(g_gen);
}

// Helper functions
namespace WeatherHelpers {
    const char* getWeatherTypeName(WeatherType type) {
        switch (type) {
            case WeatherType::CLEAR: return "Clear";
            case WeatherType::CLOUDY: return "Cloudy";
            case WeatherType::FOG: return "Fog";
            case WeatherType::RAIN: return "Rain";
            case WeatherType::HEAVY_RAIN: return "Heavy Rain";
            case WeatherType::SNOW: return "Snow";
            case WeatherType::BLIZZARD: return "Blizzard";
            case WeatherType::SANDSTORM: return "Sandstorm";
            case WeatherType::THUNDER: return "Thunderstorm";
            case WeatherType::CUSTOM: return "Custom";
            default: return "Unknown";
        }
    }
    
    WeatherPreset getDefaultPreset(WeatherType type) {
        WeatherPreset preset;
        preset.type = type;
        
        switch (type) {
            case WeatherType::CLEAR:
                preset.name = "clear";
                preset.wind.speed = 2.0f;
                preset.atmosphere.lightingMultiplier = 1.0f;
                preset.atmosphere.visibilityDistance = 10000.0f;
                break;
                
            case WeatherType::CLOUDY:
                preset.name = "cloudy";
                preset.wind.speed = 5.0f;
                preset.atmosphere.fogDensity = 0.1f;
                preset.atmosphere.lightingMultiplier = 0.7f;
                preset.atmosphere.visibilityDistance = 5000.0f;
                break;
                
            case WeatherType::RAIN:
                preset.name = "rain";
                preset.wind.speed = 8.0f;
                preset.precipitation.intensity = 0.5f;
                preset.precipitation.fallSpeed = 15.0f;
                preset.atmosphere.fogDensity = 0.2f;
                preset.atmosphere.lightingMultiplier = 0.6f;
                preset.atmosphere.visibilityDistance = 2000.0f;
                break;
                
            case WeatherType::HEAVY_RAIN:
                preset.name = "heavy_rain";
                preset.wind.speed = 15.0f;
                preset.precipitation.intensity = 1.0f;
                preset.precipitation.fallSpeed = 25.0f;
                preset.atmosphere.fogDensity = 0.4f;
                preset.atmosphere.lightingMultiplier = 0.4f;
                preset.atmosphere.visibilityDistance = 500.0f;
                break;
                
            case WeatherType::SNOW:
                preset.name = "snow";
                preset.wind.speed = 3.0f;
                preset.precipitation.intensity = 0.6f;
                preset.precipitation.fallSpeed = 2.0f;
                preset.atmosphere.fogDensity = 0.3f;
                preset.atmosphere.lightingMultiplier = 0.8f;
                preset.atmosphere.visibilityDistance = 1000.0f;
                break;
                
            case WeatherType::THUNDER:
                preset.name = "thunder";
                preset.wind.speed = 20.0f;
                preset.precipitation.intensity = 0.9f;
                preset.precipitation.fallSpeed = 30.0f;
                preset.atmosphere.fogDensity = 0.5f;
                preset.atmosphere.lightingMultiplier = 0.3f;
                preset.atmosphere.visibilityDistance = 300.0f;
                break;
                
            default:
                break;
        }
        
        return preset;
    }
}

} // namespace Graphics
} // namespace JJM
