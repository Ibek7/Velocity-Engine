#include "graphics/DynamicWeather.h"
#include <cmath>
#include <algorithm>

namespace Engine {

DynamicWeather::DynamicWeather()
    : m_lightningFrequency(0.1f)
    , m_lightningTimer(0.0f)
    , m_time(0.0f)
    , m_precipitationVAO(0)
    , m_precipitationVBO(0)
    , m_cloudShader(0)
    , m_precipShader(0)
    , m_lightningShader(0)
{
    m_currentState.type = WeatherType::Clear;
    m_currentState.transition = 1.0f;
}

DynamicWeather::~DynamicWeather() {
    // Cleanup render resources
    // glDeleteVertexArrays(1, &m_precipitationVAO);
    // glDeleteBuffers(1, &m_precipitationVBO);
}

void DynamicWeather::setWeather(WeatherType type, float transitionTime) {
    if (type == m_currentState.type) {
        return;
    }
    
    m_transition.from = m_currentState.type;
    m_transition.to = type;
    m_transition.duration = transitionTime;
    m_transition.elapsed = 0.0f;
    m_transition.active = true;
    
    m_targetState = getWeatherPreset(type);
}

void DynamicWeather::update(float deltaTime) {
    m_time += deltaTime;
    
    if (m_transition.active) {
        updateTransition(deltaTime);
    }
    
    updatePrecipitation(deltaTime);
    updateClouds(deltaTime);
    updateWind(deltaTime);
    updateFog(deltaTime);
    updateLightning(deltaTime);
    updateEffects(deltaTime);
}

bool DynamicWeather::isRaining() const {
    return m_currentState.type == WeatherType::Rain ||
           m_currentState.type == WeatherType::HeavyRain ||
           m_currentState.type == WeatherType::Thunderstorm;
}

bool DynamicWeather::isSnowing() const {
    return m_currentState.type == WeatherType::Snow ||
           m_currentState.type == WeatherType::Blizzard;
}

bool DynamicWeather::hasLightning() const {
    return m_currentState.type == WeatherType::Thunderstorm;
}

void DynamicWeather::setPrecipitation(const Precipitation& precip) {
    m_currentState.precipitation = precip;
}

void DynamicWeather::setClouds(const CloudConfig& clouds) {
    m_currentState.clouds = clouds;
}

void DynamicWeather::setWind(const Wind& wind) {
    m_currentState.wind = wind;
}

void DynamicWeather::setFog(const FogSettings& fog) {
    m_currentState.fog = fog;
}

void DynamicWeather::triggerLightning(float x, float y, float z) {
    Lightning strike;
    strike.active = true;
    strike.positionX = x;
    strike.positionY = y;
    strike.positionZ = z;
    strike.intensity = 1.0f;
    strike.duration = 0.2f;
    strike.timeRemaining = strike.duration;
    
    m_currentState.lightning.push_back(strike);
}

void DynamicWeather::setLightningFrequency(float frequency) {
    m_lightningFrequency = frequency;
}

void DynamicWeather::setTemperature(float celsius) {
    m_currentState.temperature = celsius;
}

void DynamicWeather::setHumidity(float percent) {
    m_currentState.humidity = std::clamp(percent, 0.0f, 100.0f);
}

void DynamicWeather::renderPrecipitation() {
    if (!m_currentState.precipitation.enabled) {
        return;
    }
    
    // glUseProgram(m_precipShader);
    // glBindVertexArray(m_precipitationVAO);
    
    // Set uniforms
    // glUniform1f(intensityLoc, m_currentState.precipitation.intensity);
    // glUniform1f(particleSizeLoc, m_currentState.precipitation.particleSize);
    
    // Draw particles
    // glDrawArrays(GL_POINTS, 0, m_currentState.precipitation.particleCount);
}

void DynamicWeather::renderClouds() {
    if (!m_currentState.clouds.enabled) {
        return;
    }
    
    // glUseProgram(m_cloudShader);
    
    // Set cloud parameters
    // glUniform1f(coverageLoc, m_currentState.clouds.coverage);
    // glUniform1f(densityLoc, m_currentState.clouds.density);
    // glUniform1f(timeLoc, m_time);
    
    // Render volumetric clouds or cloud plane
    // renderCloudLayers();
}

void DynamicWeather::renderLightning() {
    if (m_currentState.lightning.empty()) {
        return;
    }
    
    // glUseProgram(m_lightningShader);
    
    for (const auto& strike : m_currentState.lightning) {
        if (!strike.active) continue;
        
        // Render lightning bolt
        // glUniform3f(positionLoc, strike.positionX, strike.positionY, strike.positionZ);
        // glUniform1f(intensityLoc, strike.intensity);
        
        // Draw lightning geometry
    }
}

float DynamicWeather::getThunderVolume() const {
    if (!hasLightning()) {
        return 0.0f;
    }
    
    // Calculate volume based on active lightning strikes
    float volume = 0.0f;
    for (const auto& strike : m_currentState.lightning) {
        if (strike.active) {
            volume = std::max(volume, strike.intensity);
        }
    }
    return volume;
}

float DynamicWeather::getRainVolume() const {
    if (!isRaining()) {
        return 0.0f;
    }
    return m_currentState.precipitation.intensity;
}

float DynamicWeather::getWindVolume() const {
    return std::min(m_currentState.wind.speed / 20.0f, 1.0f);
}

void DynamicWeather::updateTransition(float deltaTime) {
    m_transition.elapsed += deltaTime;
    
    if (m_transition.elapsed >= m_transition.duration) {
        m_currentState = m_targetState;
        m_currentState.type = m_transition.to;
        m_currentState.transition = 1.0f;
        m_transition.active = false;
        return;
    }
    
    float t = m_transition.elapsed / m_transition.duration;
    m_currentState.transition = t;
    
    // Get starting state
    WeatherState fromState = getWeatherPreset(m_transition.from);
    
    // Interpolate
    interpolateStates(fromState, m_targetState, t);
    m_currentState.type = m_transition.to;
}

void DynamicWeather::updatePrecipitation(float deltaTime) {
    if (!m_currentState.precipitation.enabled) {
        return;
    }
    
    // Update particle positions based on fall speed and wind
    // This would typically be done on GPU or in a particle system
}

void DynamicWeather::updateClouds(float deltaTime) {
    if (!m_currentState.clouds.enabled) {
        return;
    }
    
    // Animate cloud movement
    // Update cloud texture coordinates or noise offsets
}

void DynamicWeather::updateWind(float deltaTime) {
    // Add gusts
    float gust = std::sin(m_time * m_currentState.wind.gustFrequency) * m_currentState.wind.gustStrength;
    float effectiveSpeed = m_currentState.wind.speed + gust;
    
    // Wind affects precipitation
    if (m_currentState.precipitation.enabled) {
        // Apply wind influence to particles
    }
}

void DynamicWeather::updateFog(float deltaTime) {
    if (!m_currentState.fog.enabled) {
        return;
    }
    
    // Update fog parameters based on weather
    // Fog density can vary with humidity and temperature
}

void DynamicWeather::updateLightning(float deltaTime) {
    if (!hasLightning()) {
        return;
    }
    
    // Update existing strikes
    for (auto it = m_currentState.lightning.begin(); it != m_currentState.lightning.end();) {
        it->timeRemaining -= deltaTime;
        if (it->timeRemaining <= 0.0f) {
            it = m_currentState.lightning.erase(it);
        } else {
            it->intensity = it->timeRemaining / it->duration;
            ++it;
        }
    }
    
    // Random lightning strikes
    m_lightningTimer += deltaTime;
    if (m_lightningTimer >= 1.0f / m_lightningFrequency) {
        m_lightningTimer = 0.0f;
        
        // Random position
        float x = (rand() % 2000 - 1000) / 10.0f;
        float y = 100.0f;
        float z = (rand() % 2000 - 1000) / 10.0f;
        
        triggerLightning(x, y, z);
    }
}

void DynamicWeather::updateEffects(float deltaTime) {
    // Update wetness
    if (isRaining()) {
        m_currentState.wetness = std::min(m_currentState.wetness + deltaTime * 0.2f, 1.0f);
    } else {
        m_currentState.wetness = std::max(m_currentState.wetness - deltaTime * 0.1f, 0.0f);
    }
    
    // Update visibility based on weather
    if (m_currentState.fog.enabled) {
        m_currentState.visibility = m_currentState.fog.endDistance;
    } else if (isRaining()) {
        m_currentState.visibility = 500.0f * (1.0f - m_currentState.precipitation.intensity * 0.5f);
    } else {
        m_currentState.visibility = 1000.0f;
    }
}

WeatherState DynamicWeather::getWeatherPreset(WeatherType type) const {
    WeatherState state;
    state.type = type;
    
    switch (type) {
        case WeatherType::Clear:
            state.clouds.coverage = 0.1f;
            state.wind.speed = 2.0f;
            state.visibility = 1000.0f;
            break;
            
        case WeatherType::Cloudy:
            state.clouds.enabled = true;
            state.clouds.coverage = 0.6f;
            state.wind.speed = 5.0f;
            state.visibility = 800.0f;
            break;
            
        case WeatherType::Overcast:
            state.clouds.enabled = true;
            state.clouds.coverage = 0.9f;
            state.wind.speed = 7.0f;
            state.visibility = 600.0f;
            break;
            
        case WeatherType::Rain:
            state.precipitation.enabled = true;
            state.precipitation.intensity = 0.5f;
            state.precipitation.particleCount = 2000;
            state.clouds.coverage = 1.0f;
            state.wind.speed = 8.0f;
            state.visibility = 400.0f;
            break;
            
        case WeatherType::HeavyRain:
            state.precipitation.enabled = true;
            state.precipitation.intensity = 1.0f;
            state.precipitation.particleCount = 5000;
            state.clouds.coverage = 1.0f;
            state.wind.speed = 12.0f;
            state.visibility = 200.0f;
            break;
            
        case WeatherType::Thunderstorm:
            state.precipitation.enabled = true;
            state.precipitation.intensity = 1.0f;
            state.precipitation.particleCount = 5000;
            state.clouds.coverage = 1.0f;
            state.wind.speed = 15.0f;
            state.visibility = 150.0f;
            break;
            
        case WeatherType::Snow:
            state.precipitation.enabled = true;
            state.precipitation.intensity = 0.5f;
            state.precipitation.particleCount = 1500;
            state.precipitation.fallSpeed = 2.0f;
            state.clouds.coverage = 0.8f;
            state.wind.speed = 5.0f;
            state.temperature = -5.0f;
            state.visibility = 300.0f;
            break;
            
        case WeatherType::Blizzard:
            state.precipitation.enabled = true;
            state.precipitation.intensity = 1.0f;
            state.precipitation.particleCount = 4000;
            state.precipitation.fallSpeed = 3.0f;
            state.clouds.coverage = 1.0f;
            state.wind.speed = 20.0f;
            state.temperature = -15.0f;
            state.visibility = 50.0f;
            break;
            
        case WeatherType::Fog:
            state.fog.enabled = true;
            state.fog.density = 0.05f;
            state.fog.startDistance = 5.0f;
            state.fog.endDistance = 100.0f;
            state.clouds.coverage = 0.5f;
            state.wind.speed = 1.0f;
            state.visibility = 100.0f;
            break;
            
        case WeatherType::Mist:
            state.fog.enabled = true;
            state.fog.density = 0.02f;
            state.fog.startDistance = 10.0f;
            state.fog.endDistance = 200.0f;
            state.clouds.coverage = 0.3f;
            state.wind.speed = 3.0f;
            state.visibility = 200.0f;
            break;
    }
    
    return state;
}

void DynamicWeather::interpolateStates(const WeatherState& from, const WeatherState& to, float t) {
    // Interpolate precipitation
    m_currentState.precipitation.intensity = from.precipitation.intensity + 
        (to.precipitation.intensity - from.precipitation.intensity) * t;
    
    // Interpolate clouds
    m_currentState.clouds.coverage = from.clouds.coverage + 
        (to.clouds.coverage - from.clouds.coverage) * t;
    m_currentState.clouds.density = from.clouds.density + 
        (to.clouds.density - from.clouds.density) * t;
    
    // Interpolate wind
    m_currentState.wind.speed = from.wind.speed + 
        (to.wind.speed - from.wind.speed) * t;
    
    // Interpolate fog
    m_currentState.fog.density = from.fog.density + 
        (to.fog.density - from.fog.density) * t;
    
    // Interpolate environmental
    m_currentState.temperature = from.temperature + 
        (to.temperature - from.temperature) * t;
    m_currentState.humidity = from.humidity + 
        (to.humidity - from.humidity) * t;
    m_currentState.visibility = from.visibility + 
        (to.visibility - from.visibility) * t;
}

// WeatherSystem implementation
WeatherSystem& WeatherSystem::getInstance() {
    static WeatherSystem instance;
    return instance;
}

void WeatherSystem::initialize() {
    m_weather = DynamicWeather();
    
    // Add default presets
    WeatherPreset clear("Clear", WeatherType::Clear);
    addPreset(clear);
    
    WeatherPreset rainy("Rainy", WeatherType::Rain);
    addPreset(rainy);
    
    WeatherPreset stormy("Stormy", WeatherType::Thunderstorm);
    addPreset(stormy);
}

void WeatherSystem::shutdown() {
    m_presets.clear();
}

void WeatherSystem::update(float deltaTime) {
    m_weather.update(deltaTime);
}

void WeatherSystem::addPreset(const WeatherPreset& preset) {
    m_presets.push_back(preset);
}

WeatherPreset* WeatherSystem::getPreset(const std::string& name) {
    for (auto& preset : m_presets) {
        if (preset.name == name) {
            return &preset;
        }
    }
    return nullptr;
}

void WeatherSystem::applyPreset(const std::string& name) {
    WeatherPreset* preset = getPreset(name);
    if (preset) {
        m_weather.setWeather(preset->type);
    }
}

void WeatherSystem::setTimeOfDay(float hours) {
    m_timeOfDay = hours;
}

} // namespace Engine
