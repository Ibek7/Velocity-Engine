#include "graphics/TimeOfDaySystem.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace JJM {
namespace Graphics {

TimeOfDaySystem::TimeOfDaySystem()
    : m_smoothTransitions(true), m_transitionTime(2.0f), m_transitionProgress(0.0f),
      m_autoProgress(true), m_useCustomConfig(false) {
    initializePresets();
}

TimeOfDaySystem::~TimeOfDaySystem() {
    shutdown();
}

void TimeOfDaySystem::initialize() {
    m_state.currentTime = 12.0f;  // Start at noon
    m_state.period = TimeOfDayPeriod::NOON;
    m_state.dayLength = 24.0f;  // 24 real minutes = 24 game hours
    m_state.timeScale = 1.0f;
    m_state.paused = false;
    
    updateLighting();
}

void TimeOfDaySystem::shutdown() {
}

void TimeOfDaySystem::update(float deltaTime) {
    if (m_state.paused || !m_autoProgress) return;
    
    updateTime(deltaTime);
    updatePeriod();
    updateSunMoonPosition();
    
    // Handle smooth transitions
    if (m_smoothTransitions && m_transitionProgress < 1.0f) {
        m_transitionProgress += deltaTime / m_transitionTime;
        m_transitionProgress = std::min(1.0f, m_transitionProgress);
        
        interpolateConfigs(m_transitionFrom, m_transitionTo, m_transitionProgress);
    }
}

void TimeOfDaySystem::setTime(float hours) {
    // Clamp to 0-24 range
    while (hours < 0) hours += 24.0f;
    while (hours >= 24.0f) hours -= 24.0f;
    
    float oldTime = m_state.currentTime;
    TimeOfDayPeriod oldPeriod = m_state.period;
    
    m_state.currentTime = hours;
    updatePeriod();
    updateLighting();
    
    notifyTimeChange(oldTime, hours);
    
    if (m_state.period != oldPeriod) {
        notifyPeriodChange(oldPeriod, m_state.period);
    }
}

std::string TimeOfDaySystem::getTimeString() const {
    return TimeOfDayHelpers::formatTime(m_state.currentTime);
}

void TimeOfDaySystem::setDayLength(float minutes) {
    m_state.dayLength = std::max(0.1f, minutes);
}

void TimeOfDaySystem::skipToNextPeriod() {
    TimeOfDayPeriod nextPeriod = static_cast<TimeOfDayPeriod>(
        (static_cast<int>(m_state.period) + 1) % 8);
    skipToPeriod(nextPeriod);
}

void TimeOfDaySystem::skipToPeriod(TimeOfDayPeriod period) {
    float startTime, endTime;
    TimeOfDayHelpers::getPeriodTimeRange(period, startTime, endTime);
    setTime(startTime);
}

void TimeOfDaySystem::updateLighting() {
    if (m_useCustomConfig) return;
    
    int periodIndex = static_cast<int>(m_state.period);
    m_state.sky = m_presets[periodIndex].sky;
    m_state.lighting = m_presets[periodIndex].lighting;
    
    updateSunMoonPosition();
}

void TimeOfDaySystem::setCustomSkyConfig(const SkyConfig& config) {
    m_state.sky = config;
    m_useCustomConfig = true;
}

void TimeOfDaySystem::setCustomLightingConfig(const LightingConfig& config) {
    m_state.lighting = config;
    m_useCustomConfig = true;
}

void TimeOfDaySystem::resetToAutomatic() {
    m_useCustomConfig = false;
    updateLighting();
}

void TimeOfDaySystem::setPreset(TimeOfDayPeriod period, const SkyConfig& sky,
                                const LightingConfig& lighting) {
    int index = static_cast<int>(period);
    m_presets[index].sky = sky;
    m_presets[index].lighting = lighting;
}

void TimeOfDaySystem::getPreset(TimeOfDayPeriod period, SkyConfig& outSky,
                                LightingConfig& outLighting) const {
    int index = static_cast<int>(period);
    outSky = m_presets[index].sky;
    outLighting = m_presets[index].lighting;
}

void TimeOfDaySystem::setTimeChangeCallback(std::function<void(const TimeChangeEvent&)> callback) {
    m_timeChangeCallback = callback;
}

void TimeOfDaySystem::setPeriodChangeCallback(std::function<void(TimeOfDayPeriod)> callback) {
    m_periodChangeCallback = callback;
}

void TimeOfDaySystem::setSunriseCallback(std::function<void()> callback) {
    m_sunriseCallback = callback;
}

void TimeOfDaySystem::setSunsetCallback(std::function<void()> callback) {
    m_sunsetCallback = callback;
}

void TimeOfDaySystem::setSmoothTransitions(bool enabled, float transitionTime) {
    m_smoothTransitions = enabled;
    m_transitionTime = transitionTime;
}

bool TimeOfDaySystem::loadState(const std::string& filepath) {
    // TODO: Implement JSON/binary loading
    return false;
}

bool TimeOfDaySystem::saveState(const std::string& filepath) {
    // TODO: Implement JSON/binary saving
    return false;
}

void TimeOfDaySystem::updateTime(float deltaTime) {
    if (m_state.paused) return;
    
    // Convert real time to game time
    // dayLength minutes = 24 game hours
    // So: game hours per real second = 24 / (dayLength * 60)
    float gameHoursPerSecond = 24.0f / (m_state.dayLength * 60.0f);
    float timeAdvance = deltaTime * gameHoursPerSecond * m_state.timeScale;
    
    float oldTime = m_state.currentTime;
    m_state.currentTime += timeAdvance;
    
    // Wrap around 24 hours
    if (m_state.currentTime >= 24.0f) {
        m_state.currentTime -= 24.0f;
    }
    
    notifyTimeChange(oldTime, m_state.currentTime);
}

void TimeOfDaySystem::updatePeriod() {
    TimeOfDayPeriod oldPeriod = m_state.period;
    m_state.period = calculatePeriod(m_state.currentTime);
    
    if (m_state.period != oldPeriod) {
        // Start transition
        if (m_smoothTransitions) {
            int oldIndex = static_cast<int>(oldPeriod);
            int newIndex = static_cast<int>(m_state.period);
            m_transitionFrom = m_presets[oldIndex];
            m_transitionTo = m_presets[newIndex];
            m_transitionProgress = 0.0f;
        }
        
        notifyPeriodChange(oldPeriod, m_state.period);
    }
}

void TimeOfDaySystem::updateSunMoonPosition() {
    // Calculate sun position based on time
    // Noon = sun at zenith (0, 1, 0)
    // Midnight = sun at nadir (0, -1, 0)
    float angle = (m_state.currentTime - 12.0f) * (M_PI / 12.0f);  // -12 to +12 hours
    
    m_state.lighting.sun.direction[0] = 0.0f;
    m_state.lighting.sun.direction[1] = -std::sin(angle);
    m_state.lighting.sun.direction[2] = std::cos(angle);
    
    // Sun is visible during day
    m_state.lighting.sun.visible = (m_state.currentTime >= 6.0f && m_state.currentTime < 18.0f);
    
    // Moon is opposite to sun
    m_state.lighting.moon.direction[0] = -m_state.lighting.sun.direction[0];
    m_state.lighting.moon.direction[1] = -m_state.lighting.sun.direction[1];
    m_state.lighting.moon.direction[2] = -m_state.lighting.sun.direction[2];
    m_state.lighting.moon.visible = !m_state.lighting.sun.visible;
}

void TimeOfDaySystem::interpolateConfigs(const PeriodPreset& from,
                                        const PeriodPreset& to, float t) {
    // Interpolate sky
    for (int i = 0; i < 3; ++i) {
        m_state.sky.zenithColor[i] = from.sky.zenithColor[i] * (1-t) + to.sky.zenithColor[i] * t;
        m_state.sky.horizonColor[i] = from.sky.horizonColor[i] * (1-t) + to.sky.horizonColor[i] * t;
        m_state.sky.fogColor[i] = from.sky.fogColor[i] * (1-t) + to.sky.fogColor[i] * t;
    }
    m_state.sky.fogDensity = from.sky.fogDensity * (1-t) + to.sky.fogDensity * t;
    m_state.sky.starVisibility = from.sky.starVisibility * (1-t) + to.sky.starVisibility * t;
    m_state.sky.cloudCoverage = from.sky.cloudCoverage * (1-t) + to.sky.cloudCoverage * t;
    
    // Interpolate lighting
    for (int i = 0; i < 3; ++i) {
        m_state.lighting.ambientColor[i] = from.lighting.ambientColor[i] * (1-t) +
                                          to.lighting.ambientColor[i] * t;
        m_state.lighting.sun.color[i] = from.lighting.sun.color[i] * (1-t) +
                                       to.lighting.sun.color[i] * t;
        m_state.lighting.moon.color[i] = from.lighting.moon.color[i] * (1-t) +
                                        to.lighting.moon.color[i] * t;
    }
    m_state.lighting.ambientIntensity = from.lighting.ambientIntensity * (1-t) +
                                       to.lighting.ambientIntensity * t;
    m_state.lighting.sun.intensity = from.lighting.sun.intensity * (1-t) +
                                    to.lighting.sun.intensity * t;
    m_state.lighting.moon.intensity = from.lighting.moon.intensity * (1-t) +
                                     to.lighting.moon.intensity * t;
}

TimeOfDayPeriod TimeOfDaySystem::calculatePeriod(float time) const {
    if (time >= 0.0f && time < 6.0f) return TimeOfDayPeriod::NIGHT;
    if (time >= 6.0f && time < 8.0f) return TimeOfDayPeriod::DAWN;
    if (time >= 8.0f && time < 12.0f) return TimeOfDayPeriod::MORNING;
    if (time >= 12.0f && time < 13.0f) return TimeOfDayPeriod::NOON;
    if (time >= 13.0f && time < 17.0f) return TimeOfDayPeriod::AFTERNOON;
    if (time >= 17.0f && time < 19.0f) return TimeOfDayPeriod::DUSK;
    if (time >= 19.0f && time < 22.0f) return TimeOfDayPeriod::EVENING;
    return TimeOfDayPeriod::LATE_NIGHT;
}

void TimeOfDaySystem::initializePresets() {
    // NIGHT (00:00 - 06:00)
    m_presets[0].sky.zenithColor[0] = 0.02f; m_presets[0].sky.zenithColor[1] = 0.02f; m_presets[0].sky.zenithColor[2] = 0.1f;
    m_presets[0].sky.starVisibility = 1.0f;
    m_presets[0].lighting.ambientIntensity = 0.1f;
    m_presets[0].lighting.moon.intensity = 0.3f;
    
    // DAWN (06:00 - 08:00)
    m_presets[1].sky.horizonColor[0] = 1.0f; m_presets[1].sky.horizonColor[1] = 0.5f; m_presets[1].sky.horizonColor[2] = 0.3f;
    m_presets[1].lighting.ambientIntensity = 0.4f;
    m_presets[1].lighting.sun.intensity = 0.5f;
    
    // MORNING (08:00 - 12:00)
    m_presets[2].sky.zenithColor[0] = 0.5f; m_presets[2].sky.zenithColor[1] = 0.7f; m_presets[2].sky.zenithColor[2] = 1.0f;
    m_presets[2].lighting.ambientIntensity = 0.7f;
    m_presets[2].lighting.sun.intensity = 1.0f;
    
    // NOON (12:00 - 13:00)
    m_presets[3].sky.zenithColor[0] = 0.4f; m_presets[3].sky.zenithColor[1] = 0.6f; m_presets[3].sky.zenithColor[2] = 1.0f;
    m_presets[3].lighting.ambientIntensity = 0.8f;
    m_presets[3].lighting.sun.intensity = 1.2f;
    
    // AFTERNOON (13:00 - 17:00)
    m_presets[4] = m_presets[2];  // Similar to morning
    
    // DUSK (17:00 - 19:00)
    m_presets[5].sky.horizonColor[0] = 1.0f; m_presets[5].sky.horizonColor[1] = 0.4f; m_presets[5].sky.horizonColor[2] = 0.2f;
    m_presets[5].lighting.ambientIntensity = 0.4f;
    m_presets[5].lighting.sun.intensity = 0.5f;
    
    // EVENING (19:00 - 22:00)
    m_presets[6].sky.zenithColor[0] = 0.1f; m_presets[6].sky.zenithColor[1] = 0.1f; m_presets[6].sky.zenithColor[2] = 0.2f;
    m_presets[6].lighting.ambientIntensity = 0.2f;
    m_presets[6].lighting.moon.intensity = 0.2f;
    m_presets[6].sky.starVisibility = 0.5f;
    
    // LATE_NIGHT (22:00 - 24:00)
    m_presets[7] = m_presets[0];  // Similar to night
}

void TimeOfDaySystem::notifyTimeChange(float oldTime, float newTime) {
    // Check for sunrise/sunset
    if (oldTime < 6.0f && newTime >= 6.0f && m_sunriseCallback) {
        m_sunriseCallback();
    }
    if (oldTime < 18.0f && newTime >= 18.0f && m_sunsetCallback) {
        m_sunsetCallback();
    }
    
    if (m_timeChangeCallback) {
        TimeChangeEvent event;
        event.oldTime = oldTime;
        event.newTime = newTime;
        event.oldPeriod = calculatePeriod(oldTime);
        event.newPeriod = calculatePeriod(newTime);
        event.periodChanged = (event.oldPeriod != event.newPeriod);
        
        m_timeChangeCallback(event);
    }
}

void TimeOfDaySystem::notifyPeriodChange(TimeOfDayPeriod oldPeriod, TimeOfDayPeriod newPeriod) {
    if (m_periodChangeCallback) {
        m_periodChangeCallback(newPeriod);
    }
}

// Helper functions
namespace TimeOfDayHelpers {
    const char* getPeriodName(TimeOfDayPeriod period) {
        switch (period) {
            case TimeOfDayPeriod::NIGHT: return "Night";
            case TimeOfDayPeriod::DAWN: return "Dawn";
            case TimeOfDayPeriod::MORNING: return "Morning";
            case TimeOfDayPeriod::NOON: return "Noon";
            case TimeOfDayPeriod::AFTERNOON: return "Afternoon";
            case TimeOfDayPeriod::DUSK: return "Dusk";
            case TimeOfDayPeriod::EVENING: return "Evening";
            case TimeOfDayPeriod::LATE_NIGHT: return "Late Night";
            default: return "Unknown";
        }
    }
    
    void getPeriodTimeRange(TimeOfDayPeriod period, float& outStart, float& outEnd) {
        switch (period) {
            case TimeOfDayPeriod::NIGHT: outStart = 0.0f; outEnd = 6.0f; break;
            case TimeOfDayPeriod::DAWN: outStart = 6.0f; outEnd = 8.0f; break;
            case TimeOfDayPeriod::MORNING: outStart = 8.0f; outEnd = 12.0f; break;
            case TimeOfDayPeriod::NOON: outStart = 12.0f; outEnd = 13.0f; break;
            case TimeOfDayPeriod::AFTERNOON: outStart = 13.0f; outEnd = 17.0f; break;
            case TimeOfDayPeriod::DUSK: outStart = 17.0f; outEnd = 19.0f; break;
            case TimeOfDayPeriod::EVENING: outStart = 19.0f; outEnd = 22.0f; break;
            case TimeOfDayPeriod::LATE_NIGHT: outStart = 22.0f; outEnd = 24.0f; break;
        }
    }
    
    std::string formatTime(float hours) {
        int h = static_cast<int>(hours);
        int m = static_cast<int>((hours - h) * 60);
        
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(2) << h << ":"
           << std::setfill('0') << std::setw(2) << m;
        return ss.str();
    }
}

} // namespace Graphics
} // namespace JJM
