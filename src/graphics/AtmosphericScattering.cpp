#include "graphics/AtmosphericScattering.h"
#include <cmath>
#include <algorithm>

namespace Engine {

AtmosphericScattering::AtmosphericScattering()
    : m_currentTime(12.0f)
    , m_weather(WeatherType::Clear)
    , m_targetWeather(WeatherType::Clear)
    , m_weatherTransitionTime(0.0f)
    , m_weatherTransitionDuration(0.0f)
    , m_skyVAO(0)
    , m_skyVBO(0)
    , m_skyIBO(0)
    , m_skyIndexCount(0)
    , m_transmittanceLUT(0)
    , m_multiScatterLUT(0)
    , m_skyViewLUT(0)
    , m_skyShader(0)
    , m_cloudShader(0)
    , m_starShader(0)
    , m_celestialShader(0)
{
}

AtmosphericScattering::~AtmosphericScattering() {
    shutdown();
}

void AtmosphericScattering::initialize() {
    createSkyDomeMesh();
    setupShaders();
    generateTransmittanceLUT();
    generateMultiScatteringLUT();
    updateSun();
    updateMoon();
}

void AtmosphericScattering::shutdown() {
    // Clean up OpenGL resources
    // glDeleteVertexArrays(1, &m_skyVAO);
    // glDeleteBuffers(1, &m_skyVBO);
    // glDeleteBuffers(1, &m_skyIBO);
    // glDeleteTextures(1, &m_transmittanceLUT);
    // glDeleteTextures(1, &m_multiScatterLUT);
    // glDeleteTextures(1, &m_skyViewLUT);
    
    m_cloudLayers.clear();
}

void AtmosphericScattering::setParameters(const AtmosphereParameters& params) {
    m_params = params;
    generateTransmittanceLUT();
}

void AtmosphericScattering::setSettings(const SkyDomeSettings& settings) {
    m_settings = settings;
}

void AtmosphericScattering::setTime(float hours) {
    m_currentTime = std::fmod(hours, 24.0f);
    if (m_currentTime < 0.0f) m_currentTime += 24.0f;
    
    updateSun();
    updateMoon();
}

TimeOfDay AtmosphericScattering::getTimeOfDay() const {
    if (m_currentTime >= 5.0f && m_currentTime < 7.0f) return TimeOfDay::Dawn;
    if (m_currentTime >= 7.0f && m_currentTime < 11.0f) return TimeOfDay::Morning;
    if (m_currentTime >= 11.0f && m_currentTime < 14.0f) return TimeOfDay::Noon;
    if (m_currentTime >= 14.0f && m_currentTime < 18.0f) return TimeOfDay::Afternoon;
    if (m_currentTime >= 18.0f && m_currentTime < 20.0f) return TimeOfDay::Dusk;
    return TimeOfDay::Night;
}

void AtmosphericScattering::setTimeOfDay(TimeOfDay tod) {
    switch (tod) {
        case TimeOfDay::Dawn: setTime(6.0f); break;
        case TimeOfDay::Morning: setTime(9.0f); break;
        case TimeOfDay::Noon: setTime(12.0f); break;
        case TimeOfDay::Afternoon: setTime(15.0f); break;
        case TimeOfDay::Dusk: setTime(19.0f); break;
        case TimeOfDay::Night: setTime(0.0f); break;
    }
}

void AtmosphericScattering::setWeather(WeatherType weather) {
    m_weather = weather;
    
    // Adjust atmospheric parameters based on weather
    switch (weather) {
        case WeatherType::Clear:
            m_params.mieScattering = 21e-6f;
            m_params.mieG = 0.76f;
            break;
        case WeatherType::Cloudy:
            m_params.mieScattering = 30e-6f;
            m_params.mieG = 0.80f;
            break;
        case WeatherType::Overcast:
            m_params.mieScattering = 45e-6f;
            m_params.mieG = 0.85f;
            break;
        case WeatherType::Rainy:
            m_params.mieScattering = 60e-6f;
            m_params.mieG = 0.90f;
            break;
        case WeatherType::Stormy:
            m_params.mieScattering = 80e-6f;
            m_params.mieG = 0.92f;
            break;
        case WeatherType::Foggy:
            m_params.mieScattering = 100e-6f;
            m_params.mieG = 0.95f;
            break;
        case WeatherType::Snowy:
            m_params.mieScattering = 50e-6f;
            m_params.mieG = 0.88f;
            break;
    }
}

void AtmosphericScattering::transitionWeather(WeatherType target, float duration) {
    m_targetWeather = target;
    m_weatherTransitionTime = 0.0f;
    m_weatherTransitionDuration = duration;
}

void AtmosphericScattering::updateSun() {
    float azimuth, elevation;
    calculateSunPosition(m_currentTime, azimuth, elevation);
    
    m_sun.name = "Sun";
    m_sun.azimuth = azimuth;
    m_sun.elevation = elevation;
    m_sun.angularSize = m_params.sunAngularSize;
    m_sun.intensity = m_params.sunIntensity;
    
    // Adjust sun color based on elevation
    if (elevation < 0.0f) {
        // Below horizon
        m_sun.intensity = 0.0f;
    } else if (elevation < 0.1f) {
        // Sunrise/sunset - orange/red
        float t = elevation / 0.1f;
        m_sun.colorR = 1.0f;
        m_sun.colorG = 0.3f + 0.7f * t;
        m_sun.colorB = 0.1f + 0.9f * t;
        m_sun.intensity = m_params.sunIntensity * t;
    } else {
        // Day - white/yellow
        m_sun.colorR = 1.0f;
        m_sun.colorG = 1.0f;
        m_sun.colorB = 0.95f;
    }
}

void AtmosphericScattering::updateMoon() {
    float azimuth, elevation;
    calculateMoonPosition(m_currentTime, azimuth, elevation);
    
    m_moon.name = "Moon";
    m_moon.azimuth = azimuth;
    m_moon.elevation = elevation;
    m_moon.angularSize = 0.52f;
    m_moon.colorR = 0.8f;
    m_moon.colorG = 0.8f;
    m_moon.colorB = 0.9f;
    
    if (elevation > 0.0f) {
        m_moon.intensity = 0.1f * std::min(1.0f, elevation / 0.5f);
    } else {
        m_moon.intensity = 0.0f;
    }
}

void AtmosphericScattering::addCloudLayer(const CloudLayer& layer) {
    m_cloudLayers.push_back(layer);
}

void AtmosphericScattering::removeCloudLayer(int index) {
    if (index >= 0 && index < static_cast<int>(m_cloudLayers.size())) {
        m_cloudLayers.erase(m_cloudLayers.begin() + index);
    }
}

void AtmosphericScattering::updateClouds(float deltaTime) {
    for (auto& layer : m_cloudLayers) {
        // Animate cloud movement (would update noise offset in real implementation)
    }
}

void AtmosphericScattering::calculateInscattering(float viewDirX, float viewDirY, float viewDirZ,
                                                  float& outR, float& outG, float& outB) const {
    // Simplified inscattering calculation
    float sunDirX = std::cos(m_sun.azimuth) * std::cos(m_sun.elevation);
    float sunDirY = std::sin(m_sun.elevation);
    float sunDirZ = std::sin(m_sun.azimuth) * std::cos(m_sun.elevation);
    
    float cosTheta = viewDirX * sunDirX + viewDirY * sunDirY + viewDirZ * sunDirZ;
    
    // Rayleigh scattering
    float phaseR = calculatePhaseRayleigh(cosTheta);
    float rayleighR = m_params.rayleighR * phaseR;
    float rayleighG = m_params.rayleighG * phaseR;
    float rayleighB = m_params.rayleighB * phaseR;
    
    // Mie scattering
    float phaseM = calculatePhaseMie(cosTheta, m_params.mieG);
    float mie = m_params.mieScattering * phaseM;
    
    outR = (rayleighR + mie) * m_sun.intensity;
    outG = (rayleighG + mie) * m_sun.intensity;
    outB = (rayleighB + mie) * m_sun.intensity;
}

void AtmosphericScattering::calculateTransmittance(float viewDirX, float viewDirY, float viewDirZ,
                                                   float distance, float& outR, float& outG, float& outB) const {
    // Simplified transmittance
    float depth = distance / 1000.0f; // Convert to km
    
    float attenuationR = std::exp(-m_params.rayleighR * depth);
    float attenuationG = std::exp(-m_params.rayleighG * depth);
    float attenuationB = std::exp(-m_params.rayleighB * depth);
    
    outR = attenuationR;
    outG = attenuationG;
    outB = attenuationB;
}

float AtmosphericScattering::calculatePhaseRayleigh(float cosTheta) const {
    // Rayleigh phase function
    return 3.0f / (16.0f * 3.14159f) * (1.0f + cosTheta * cosTheta);
}

float AtmosphericScattering::calculatePhaseMie(float cosTheta, float g) const {
    // Henyey-Greenstein phase function
    float g2 = g * g;
    float denom = 1.0f + g2 - 2.0f * g * cosTheta;
    return (1.0f - g2) / (4.0f * 3.14159f * std::pow(denom, 1.5f));
}

bool AtmosphericScattering::rayIntersectSphere(float rayX, float rayY, float rayZ,
                                               float dirX, float dirY, float dirZ,
                                               float radius, float& t) const {
    float a = dirX * dirX + dirY * dirY + dirZ * dirZ;
    float b = 2.0f * (rayX * dirX + rayY * dirY + rayZ * dirZ);
    float c = rayX * rayX + rayY * rayY + rayZ * rayZ - radius * radius;
    
    float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) return false;
    
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    if (t < 0.0f) {
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
    }
    
    return t >= 0.0f;
}

float AtmosphericScattering::atmosphericDepth(float rayX, float rayY, float rayZ,
                                              float dirX, float dirY, float dirZ,
                                              float scaleHeight) const {
    float t;
    if (!rayIntersectSphere(rayX, rayY, rayZ, dirX, dirY, dirZ, m_params.atmosphereRadius, t)) {
        return 0.0f;
    }
    
    // Simplified optical depth calculation
    float distance = t;
    float height = std::sqrt(rayX * rayX + rayY * rayY + rayZ * rayZ) - m_params.planetRadius;
    
    return distance * std::exp(-height / scaleHeight);
}

void AtmosphericScattering::render(float cameraX, float cameraY, float cameraZ) {
    renderSkyDome();
    
    if (m_settings.renderClouds) {
        renderClouds();
    }
    
    if (m_settings.renderStars && getTimeOfDay() == TimeOfDay::Night) {
        renderStars();
    }
    
    renderCelestialBodies();
}

void AtmosphericScattering::renderSkyDome() {
    // Bind sky shader and render dome
    // glUseProgram(m_skyShader);
    // glBindVertexArray(m_skyVAO);
    // glDrawElements(GL_TRIANGLES, m_skyIndexCount, GL_UNSIGNED_INT, 0);
}

void AtmosphericScattering::renderClouds() {
    // Render volumetric clouds
}

void AtmosphericScattering::renderStars() {
    // Render star field
}

void AtmosphericScattering::renderCelestialBodies() {
    // Render sun and moon
}

void AtmosphericScattering::setupShaders() {
    // Load and compile shaders
}

void AtmosphericScattering::updateUniforms() {
    // Update shader uniforms with current parameters
}

void AtmosphericScattering::generateTransmittanceLUT() {
    // Generate transmittance lookup texture
}

void AtmosphericScattering::generateMultiScatteringLUT() {
    // Generate multi-scattering lookup texture
}

void AtmosphericScattering::generateSkyViewLUT() {
    // Generate sky view lookup texture
}

void AtmosphericScattering::update(float deltaTime) {
    if (m_settings.dynamicTimeOfDay) {
        m_currentTime += deltaTime * m_settings.timeSpeed / 3600.0f; // Convert seconds to hours
        if (m_currentTime >= 24.0f) m_currentTime -= 24.0f;
        
        updateSun();
        updateMoon();
    }
    
    if (m_weatherTransitionDuration > 0.0f) {
        m_weatherTransitionTime += deltaTime;
        float t = std::min(1.0f, m_weatherTransitionTime / m_weatherTransitionDuration);
        interpolateWeather(t);
        
        if (t >= 1.0f) {
            m_weather = m_targetWeather;
            m_weatherTransitionDuration = 0.0f;
        }
    }
    
    if (m_settings.renderClouds) {
        updateClouds(deltaTime);
    }
}

void AtmosphericScattering::calculateSunPosition(float time, float& azimuth, float& elevation) const {
    // Simplified sun position calculation
    // Hour angle: 0 at noon, increases through day
    float hourAngle = (time - 12.0f) * 15.0f * 3.14159f / 180.0f; // degrees to radians
    
    // Solar declination (simplified, assumes equinox)
    float declination = 0.0f;
    
    // Observer latitude (simplified, assumes equator)
    float latitude = 0.0f;
    
    // Calculate elevation
    elevation = std::asin(std::sin(latitude) * std::sin(declination) +
                         std::cos(latitude) * std::cos(declination) * std::cos(hourAngle));
    
    // Calculate azimuth
    azimuth = std::atan2(std::sin(hourAngle),
                        std::cos(hourAngle) * std::sin(latitude) - 
                        std::tan(declination) * std::cos(latitude));
}

void AtmosphericScattering::calculateMoonPosition(float time, float& azimuth, float& elevation) const {
    // Moon is roughly opposite to sun, with 12-hour offset
    float moonTime = time + 12.0f;
    if (moonTime >= 24.0f) moonTime -= 24.0f;
    
    calculateSunPosition(moonTime, azimuth, elevation);
}

void AtmosphericScattering::interpolateWeather(float t) {
    // Interpolate weather parameters during transition
}

void AtmosphericScattering::createSkyDomeMesh() {
    // Create hemisphere mesh for sky rendering
    // Would generate vertex and index buffers
}

void AtmosphericScattering::createCloudMesh() {
    // Create mesh for cloud rendering
}

// AtmosphericSystem implementation
AtmosphericSystem& AtmosphericSystem::getInstance() {
    static AtmosphericSystem instance;
    return instance;
}

void AtmosphericSystem::initialize() {
    m_scattering.initialize();
}

void AtmosphericSystem::shutdown() {
    m_scattering.shutdown();
}

} // namespace Engine
