#include "scene/WeatherSystem.h"
#include <cmath>
#include <random>

namespace Engine {

WeatherSystem::WeatherSystem()
    : m_isTransitioning(false)
    , m_timeOfDay(12.0f)
    , m_dayNightSpeed(1.0f)
    , m_randomWeather(false)
    , m_randomWeatherTimer(0.0f)
    , m_minWeatherDuration(300.0f)    // 5 minutes
    , m_maxWeatherDuration(1200.0f)   // 20 minutes
    , m_dynamicSkybox(true)
    , m_particleEffects(true)
    , m_soundEffects(true)
{
    // Initialize with clear weather
    m_currentCondition = createConditionForType(WeatherType::Clear);
    m_targetCondition = m_currentCondition;
}

WeatherSystem& WeatherSystem::getInstance() {
    static WeatherSystem instance;
    return instance;
}

void WeatherSystem::update(float deltaTime) {
    updateTransition(deltaTime);
    updateDayNightCycle(deltaTime);
    updateRandomWeather(deltaTime);
    applyWeatherEffects();
}

void WeatherSystem::setWeather(WeatherType type, float transitionTime) {
    transitionToWeather(type, transitionTime);
}

void WeatherSystem::setWeatherImmediate(WeatherType type) {
    m_currentCondition = createConditionForType(type);
    m_targetCondition = m_currentCondition;
    m_isTransitioning = false;
    
    if (m_onWeatherChanged) {
        m_onWeatherChanged(type);
    }
}

void WeatherSystem::setIntensity(float intensity) {
    m_currentCondition.intensity = std::max(0.0f, std::min(1.0f, intensity));
}

void WeatherSystem::setWindSpeed(float speed) {
    m_currentCondition.windSpeed = std::max(0.0f, speed);
}

void WeatherSystem::setWindDirection(float degrees) {
    m_currentCondition.windDirection = fmod(degrees, 360.0f);
    if (m_currentCondition.windDirection < 0) {
        m_currentCondition.windDirection += 360.0f;
    }
}

void WeatherSystem::setTemperature(float celsius) {
    m_currentCondition.temperature = celsius;
}

void WeatherSystem::setTimeOfDay(float hours) {
    m_timeOfDay = fmod(hours, 24.0f);
    if (m_timeOfDay < 0) {
        m_timeOfDay += 24.0f;
    }
    updateAmbientLight();
}

bool WeatherSystem::isRaining() const {
    return m_currentCondition.type == WeatherType::Rainy && 
           m_currentCondition.precipitation > 0.1f;
}

bool WeatherSystem::isStormy() const {
    return m_currentCondition.type == WeatherType::Stormy;
}

bool WeatherSystem::isSnowing() const {
    return m_currentCondition.type == WeatherType::Snowy;
}

bool WeatherSystem::isClear() const {
    return m_currentCondition.type == WeatherType::Clear;
}

float WeatherSystem::getLightingMultiplier() const {
    float multiplier = 1.0f;
    
    // Weather darkness
    switch (m_currentCondition.type) {
        case WeatherType::Clear:
            multiplier = 1.0f;
            break;
        case WeatherType::Cloudy:
            multiplier = 0.8f;
            break;
        case WeatherType::Rainy:
            multiplier = 0.6f;
            break;
        case WeatherType::Stormy:
            multiplier = 0.4f;
            break;
        case WeatherType::Snowy:
            multiplier = 0.9f;
            break;
        case WeatherType::Foggy:
            multiplier = 0.7f;
            break;
        case WeatherType::Windy:
            multiplier = 0.95f;
            break;
    }
    
    // Time of day
    multiplier *= getAmbientLightLevel();
    
    return multiplier;
}

float WeatherSystem::getAmbientLightLevel() const {
    // Sine curve for day/night cycle
    // Peak at noon (12), minimum at midnight (0/24)
    float angle = (m_timeOfDay - 6.0f) * (3.14159f / 12.0f);
    float level = (sin(angle) + 1.0f) * 0.5f;
    
    // Clamp to minimum night level
    return std::max(0.1f, level);
}

void WeatherSystem::setRandomWeatherInterval(float minSeconds, float maxSeconds) {
    m_minWeatherDuration = minSeconds;
    m_maxWeatherDuration = maxSeconds;
}

void WeatherSystem::onWeatherChanged(const std::function<void(WeatherType)>& callback) {
    m_onWeatherChanged = callback;
}

void WeatherSystem::updateTransition(float deltaTime) {
    if (!m_isTransitioning) {
        return;
    }
    
    m_activeTransition.elapsed += deltaTime;
    float t = m_activeTransition.elapsed / m_activeTransition.duration;
    
    if (t >= 1.0f) {
        // Transition complete
        m_currentCondition = m_targetCondition;
        m_isTransitioning = false;
        
        if (m_onWeatherChanged) {
            m_onWeatherChanged(m_currentCondition.type);
        }
    } else {
        // Interpolate
        WeatherCondition from = createConditionForType(m_activeTransition.fromType);
        m_currentCondition = interpolateConditions(from, m_targetCondition, t);
    }
}

void WeatherSystem::updateDayNightCycle(float deltaTime) {
    m_timeOfDay += (deltaTime / 3600.0f) * m_dayNightSpeed;
    if (m_timeOfDay >= 24.0f) {
        m_timeOfDay -= 24.0f;
    }
    updateAmbientLight();
}

void WeatherSystem::updateRandomWeather(float deltaTime) {
    if (!m_randomWeather) {
        return;
    }
    
    m_randomWeatherTimer += deltaTime;
    
    // Random duration between min and max
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(m_minWeatherDuration, m_maxWeatherDuration);
    
    float targetDuration = dist(gen);
    
    if (m_randomWeatherTimer >= targetDuration) {
        m_randomWeatherTimer = 0.0f;
        
        // Pick random weather
        std::uniform_int_distribution<int> weatherDist(0, 6);
        WeatherType newWeather = static_cast<WeatherType>(weatherDist(gen));
        
        setWeather(newWeather, 10.0f); // 10 second transition
    }
}

void WeatherSystem::applyWeatherEffects() {
    // TODO: Apply visual effects (skybox, particles, fog)
    // TODO: Apply audio effects (wind, rain sounds)
}

void WeatherSystem::updateAmbientLight() {
    // Update global lighting based on time and weather
    // TODO: Set ambient light in rendering system
}

void WeatherSystem::transitionToWeather(WeatherType type, float duration) {
    m_targetCondition = createConditionForType(type);
    m_activeTransition.fromType = m_currentCondition.type;
    m_activeTransition.toType = type;
    m_activeTransition.duration = duration;
    m_activeTransition.elapsed = 0.0f;
    m_isTransitioning = true;
}

WeatherCondition WeatherSystem::createConditionForType(WeatherType type) {
    WeatherCondition condition;
    condition.type = type;
    
    switch (type) {
        case WeatherType::Clear:
            condition.intensity = 0.0f;
            condition.windSpeed = 2.0f;
            condition.precipitation = 0.0f;
            condition.temperature = 20.0f;
            condition.humidity = 0.5f;
            condition.visibility = 1.0f;
            break;
            
        case WeatherType::Cloudy:
            condition.intensity = 0.5f;
            condition.windSpeed = 5.0f;
            condition.precipitation = 0.0f;
            condition.temperature = 18.0f;
            condition.humidity = 0.6f;
            condition.visibility = 0.9f;
            break;
            
        case WeatherType::Rainy:
            condition.intensity = 0.7f;
            condition.windSpeed = 8.0f;
            condition.precipitation = 0.6f;
            condition.temperature = 15.0f;
            condition.humidity = 0.9f;
            condition.visibility = 0.7f;
            break;
            
        case WeatherType::Stormy:
            condition.intensity = 1.0f;
            condition.windSpeed = 15.0f;
            condition.precipitation = 0.9f;
            condition.temperature = 12.0f;
            condition.humidity = 0.95f;
            condition.visibility = 0.5f;
            break;
            
        case WeatherType::Snowy:
            condition.intensity = 0.6f;
            condition.windSpeed = 6.0f;
            condition.precipitation = 0.5f;
            condition.temperature = -2.0f;
            condition.humidity = 0.8f;
            condition.visibility = 0.6f;
            break;
            
        case WeatherType::Foggy:
            condition.intensity = 0.8f;
            condition.windSpeed = 1.0f;
            condition.precipitation = 0.0f;
            condition.temperature = 10.0f;
            condition.humidity = 0.95f;
            condition.visibility = 0.3f;
            break;
            
        case WeatherType::Windy:
            condition.intensity = 0.4f;
            condition.windSpeed = 20.0f;
            condition.precipitation = 0.0f;
            condition.temperature = 16.0f;
            condition.humidity = 0.5f;
            condition.visibility = 0.95f;
            break;
    }
    
    condition.windDirection = 90.0f; // East by default
    return condition;
}

WeatherCondition WeatherSystem::interpolateConditions(const WeatherCondition& from,
                                                      const WeatherCondition& to,
                                                      float t) {
    WeatherCondition result;
    result.type = to.type; // Type switches immediately
    result.intensity = from.intensity + (to.intensity - from.intensity) * t;
    result.windSpeed = from.windSpeed + (to.windSpeed - from.windSpeed) * t;
    result.windDirection = from.windDirection + (to.windDirection - from.windDirection) * t;
    result.precipitation = from.precipitation + (to.precipitation - from.precipitation) * t;
    result.temperature = from.temperature + (to.temperature - from.temperature) * t;
    result.humidity = from.humidity + (to.humidity - from.humidity) * t;
    result.visibility = from.visibility + (to.visibility - from.visibility) * t;
    return result;
}

} // namespace Engine
