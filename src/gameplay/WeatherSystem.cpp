#include "gameplay/WeatherSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace Engine {

WeatherSystem::WeatherSystem()
    : m_transitionTime(0.0f)
    , m_transitionProgress(1.0f)
    , m_dynamicEnabled(false)
    , m_changeInterval(300.0f) // 5 minutes
    , m_timeSinceLastChange(0.0f)
    , m_lightningTimer(0.0f)
    , m_lightningInterval(5.0f)
{
    // Default clear weather
    m_current.type = WeatherType::Clear;
    m_current.intensity = 0.5f;
    m_current.windSpeed = 2.0f;
    m_current.windDirection = 0.0f;
    m_current.temperature = 20.0f;
    m_current.humidity = 0.5f;
    m_current.visibility = 10000.0f;
    m_current.lightning = false;
    
    m_target = m_current;
}

WeatherSystem& WeatherSystem::getInstance() {
    static WeatherSystem instance;
    return instance;
}

void WeatherSystem::setWeather(WeatherType type, float transitionTime) {
    m_target.type = type;
    m_transitionTime = transitionTime;
    m_transitionProgress = 0.0f;
    
    // Set target conditions based on weather type
    switch (type) {
        case WeatherType::Clear:
            m_target.intensity = 0.0f;
            m_target.windSpeed = 2.0f;
            m_target.visibility = 10000.0f;
            m_target.humidity = 0.4f;
            m_target.lightning = false;
            break;
            
        case WeatherType::Cloudy:
            m_target.intensity = 0.3f;
            m_target.windSpeed = 5.0f;
            m_target.visibility = 8000.0f;
            m_target.humidity = 0.6f;
            m_target.lightning = false;
            break;
            
        case WeatherType::Rainy:
            m_target.intensity = 0.6f;
            m_target.windSpeed = 8.0f;
            m_target.visibility = 3000.0f;
            m_target.humidity = 0.9f;
            m_target.lightning = false;
            break;
            
        case WeatherType::Stormy:
            m_target.intensity = 0.9f;
            m_target.windSpeed = 15.0f;
            m_target.visibility = 1000.0f;
            m_target.humidity = 0.95f;
            m_target.lightning = true;
            break;
            
        case WeatherType::Snowy:
            m_target.intensity = 0.5f;
            m_target.windSpeed = 6.0f;
            m_target.visibility = 2000.0f;
            m_target.humidity = 0.8f;
            m_target.temperature = -5.0f;
            m_target.lightning = false;
            break;
            
        case WeatherType::Foggy:
            m_target.intensity = 0.7f;
            m_target.windSpeed = 1.0f;
            m_target.visibility = 500.0f;
            m_target.humidity = 0.95f;
            m_target.lightning = false;
            break;
    }
}

void WeatherSystem::setRandomWeather() {
    WeatherType types[] = {
        WeatherType::Clear,
        WeatherType::Cloudy,
        WeatherType::Rainy,
        WeatherType::Stormy,
        WeatherType::Snowy,
        WeatherType::Foggy
    };
    
    int index = rand() % 6;
    setWeather(types[index], 10.0f);
}

void WeatherSystem::update(float deltaTime) {
    // Transition to target weather
    if (m_transitionProgress < 1.0f) {
        transitionWeather(deltaTime);
    }
    
    // Dynamic weather changes
    if (m_dynamicEnabled) {
        updateDynamicWeather(deltaTime);
    }
    
    // Lightning for storms
    if (m_current.lightning) {
        m_lightningTimer -= deltaTime;
        if (m_lightningTimer <= 0.0f) {
            triggerLightning();
        }
    }
}

void WeatherSystem::transitionWeather(float deltaTime) {
    if (m_transitionTime <= 0.0f) {
        m_current = m_target;
        m_transitionProgress = 1.0f;
        
        // Notify callbacks
        for (auto& callback : m_callbacks) {
            callback(m_current);
        }
        return;
    }
    
    m_transitionProgress += deltaTime / m_transitionTime;
    m_transitionProgress = std::min(m_transitionProgress, 1.0f);
    
    float t = m_transitionProgress;
    
    // Smooth interpolation
    t = t * t * (3.0f - 2.0f * t);
    
    // Lerp all parameters
    m_current.intensity = m_current.intensity + (m_target.intensity - m_current.intensity) * t;
    m_current.windSpeed = m_current.windSpeed + (m_target.windSpeed - m_current.windSpeed) * t;
    m_current.visibility = m_current.visibility + (m_target.visibility - m_current.visibility) * t;
    m_current.humidity = m_current.humidity + (m_target.humidity - m_current.humidity) * t;
    m_current.temperature = m_current.temperature + (m_target.temperature - m_current.temperature) * t;
    
    if (t >= 1.0f) {
        m_current.type = m_target.type;
        m_current.lightning = m_target.lightning;
    }
}

void WeatherSystem::updateDynamicWeather(float deltaTime) {
    m_timeSinceLastChange += deltaTime;
    
    if (m_timeSinceLastChange >= m_changeInterval) {
        setRandomWeather();
        m_timeSinceLastChange = 0.0f;
    }
}

void WeatherSystem::triggerLightning() {
    // Reset timer with some randomness
    m_lightningInterval = 2.0f + (rand() % 100) / 10.0f; // 2-12 seconds
    m_lightningTimer = m_lightningInterval;
}

void WeatherSystem::setIntensity(float intensity) {
    m_current.intensity = std::max(0.0f, std::min(1.0f, intensity));
}

void WeatherSystem::setWindSpeed(float speed) {
    m_current.windSpeed = std::max(0.0f, speed);
}

void WeatherSystem::setWindDirection(float degrees) {
    m_current.windDirection = fmod(degrees, 360.0f);
    if (m_current.windDirection < 0.0f) {
        m_current.windDirection += 360.0f;
    }
}

void WeatherSystem::onWeatherChange(WeatherChangeCallback callback) {
    m_callbacks.push_back(callback);
}

float WeatherSystem::getRainAmount() const {
    if (m_current.type == WeatherType::Rainy || m_current.type == WeatherType::Stormy) {
        return m_current.intensity;
    }
    return 0.0f;
}

float WeatherSystem::getSnowAmount() const {
    if (m_current.type == WeatherType::Snowy) {
        return m_current.intensity;
    }
    return 0.0f;
}

float WeatherSystem::getFogAmount() const {
    if (m_current.type == WeatherType::Foggy) {
        return m_current.intensity;
    }
    
    // Slight fog in other weather
    return std::max(0.0f, (1.0f - m_current.visibility / 10000.0f) * 0.3f);
}

bool WeatherSystem::shouldSpawnLightning() const {
    return m_current.lightning && m_lightningTimer <= 0.0f;
}

} // namespace Engine
