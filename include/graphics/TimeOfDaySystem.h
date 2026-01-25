#pragma once

#include <string>
#include <functional>

namespace JJM {
namespace Graphics {

/**
 * Time of day periods
 */
enum class TimeOfDayPeriod {
    NIGHT,          // 00:00 - 06:00
    DAWN,           // 06:00 - 08:00
    MORNING,        // 08:00 - 12:00
    NOON,           // 12:00 - 13:00
    AFTERNOON,      // 13:00 - 17:00
    DUSK,           // 17:00 - 19:00
    EVENING,        // 19:00 - 22:00
    LATE_NIGHT      // 22:00 - 24:00
};

/**
 * Sun/Moon configuration
 */
struct CelestialBody {
    float direction[3] = {0, -1, 0};    // Direction vector
    float color[3] = {1, 1, 1};         // RGB color
    float intensity = 1.0f;
    float size = 1.0f;
    bool visible = true;
};

/**
 * Sky configuration
 */
struct SkyConfig {
    float zenithColor[3] = {0.53f, 0.81f, 0.98f};      // Top of sky
    float horizonColor[3] = {0.93f, 0.95f, 1.0f};      // Horizon
    float fogColor[3] = {0.7f, 0.8f, 0.9f};            // Fog color
    float fogDensity = 0.0f;                            // 0-1
    float starVisibility = 0.0f;                        // 0-1
    float cloudCoverage = 0.3f;                         // 0-1
};

/**
 * Lighting environment configuration
 */
struct LightingConfig {
    float ambientColor[3] = {0.4f, 0.4f, 0.5f};
    float ambientIntensity = 0.5f;
    
    CelestialBody sun;
    CelestialBody moon;
    
    float shadowStrength = 1.0f;                        // Shadow opacity
    float bloomIntensity = 0.2f;                        // Bloom effect
    float exposure = 1.0f;                              // Camera exposure
};

/**
 * Complete time of day state
 */
struct TimeOfDayState {
    float currentTime = 12.0f;          // Current time in hours (0-24)
    TimeOfDayPeriod period = TimeOfDayPeriod::NOON;
    
    SkyConfig sky;
    LightingConfig lighting;
    
    float dayLength = 24.0f;            // Real-time minutes for full day
    bool paused = false;
    float timeScale = 1.0f;             // Time speed multiplier
};

/**
 * Time change event
 */
struct TimeChangeEvent {
    float oldTime;
    float newTime;
    TimeOfDayPeriod oldPeriod;
    TimeOfDayPeriod newPeriod;
    bool periodChanged;
};

/**
 * System for managing day/night cycles and atmospheric lighting
 */
class TimeOfDaySystem {
public:
    TimeOfDaySystem();
    ~TimeOfDaySystem();
    
    /**
     * Initialize the system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update time progression and lighting
     * @param deltaTime Real-time seconds elapsed
     */
    void update(float deltaTime);
    
    // Time control
    
    /**
     * Set current time of day
     * @param hours Time in hours (0-24)
     */
    void setTime(float hours);
    
    /**
     * Get current time
     * @return Time in hours (0-24)
     */
    float getTime() const { return m_state.currentTime; }
    
    /**
     * Get current time formatted as string
     * @return Time string (e.g., "14:30")
     */
    std::string getTimeString() const;
    
    /**
     * Get current period
     * @return Time of day period
     */
    TimeOfDayPeriod getPeriod() const { return m_state.period; }
    
    /**
     * Set day length (real-time minutes for full 24h cycle)
     * @param minutes Day length in real-time minutes
     */
    void setDayLength(float minutes);
    
    /**
     * Get day length
     * @return Day length in real-time minutes
     */
    float getDayLength() const { return m_state.dayLength; }
    
    /**
     * Set time scale multiplier
     * @param scale Time speed (1.0 = normal, 2.0 = double speed, etc.)
     */
    void setTimeScale(float scale) { m_state.timeScale = scale; }
    
    /**
     * Get time scale
     * @return Current time scale
     */
    float getTimeScale() const { return m_state.timeScale; }
    
    /**
     * Pause time progression
     */
    void pause() { m_state.paused = true; }
    
    /**
     * Resume time progression
     */
    void resume() { m_state.paused = false; }
    
    /**
     * Check if time is paused
     * @return True if paused
     */
    bool isPaused() const { return m_state.paused; }
    
    /**
     * Skip to next period
     */
    void skipToNextPeriod();
    
    /**
     * Skip to specific period
     * @param period Target period
     */
    void skipToPeriod(TimeOfDayPeriod period);
    
    // Lighting and atmosphere
    
    /**
     * Get current lighting configuration
     * @return Current lighting config
     */
    const LightingConfig& getLighting() const { return m_state.lighting; }
    
    /**
     * Get current sky configuration
     * @return Current sky config
     */
    const SkyConfig& getSky() const { return m_state.sky; }
    
    /**
     * Get sun configuration
     * @return Sun celestial body
     */
    const CelestialBody& getSun() const { return m_state.lighting.sun; }
    
    /**
     * Get moon configuration
     * @return Moon celestial body
     */
    const CelestialBody& getMoon() const { return m_state.lighting.moon; }
    
    /**
     * Force update lighting (without time progression)
     */
    void updateLighting();
    
    /**
     * Set custom sky config for current time
     * @param config Sky configuration
     */
    void setCustomSkyConfig(const SkyConfig& config);
    
    /**
     * Set custom lighting config for current time
     * @param config Lighting configuration
     */
    void setCustomLightingConfig(const LightingConfig& config);
    
    /**
     * Reset to automatic sky/lighting based on time
     */
    void resetToAutomatic();
    
    // Presets
    
    /**
     * Configure preset for a time period
     * @param period Time period
     * @param sky Sky configuration
     * @param lighting Lighting configuration
     */
    void setPreset(TimeOfDayPeriod period, const SkyConfig& sky, const LightingConfig& lighting);
    
    /**
     * Get preset for a time period
     * @param period Time period
     * @param outSky Output sky config
     * @param outLighting Output lighting config
     */
    void getPreset(TimeOfDayPeriod period, SkyConfig& outSky, LightingConfig& outLighting) const;
    
    // Callbacks
    
    /**
     * Set callback for time changes
     * @param callback Function called when time updates
     */
    void setTimeChangeCallback(std::function<void(const TimeChangeEvent&)> callback);
    
    /**
     * Set callback for period changes
     * @param callback Function called when period changes
     */
    void setPeriodChangeCallback(std::function<void(TimeOfDayPeriod)> callback);
    
    /**
     * Set callback for sunrise
     * @param callback Function called at sunrise
     */
    void setSunriseCallback(std::function<void()> callback);
    
    /**
     * Set callback for sunset
     * @param callback Function called at sunset
     */
    void setSunsetCallback(std::function<void()> callback);
    
    // Configuration
    
    /**
     * Enable or disable automatic time progression
     * @param enabled True to enable
     */
    void setAutoProgress(bool enabled) { m_autoProgress = enabled; }
    
    /**
     * Check if auto-progress is enabled
     * @return True if enabled
     */
    bool isAutoProgressEnabled() const { return m_autoProgress; }
    
    /**
     * Enable or disable smooth transitions
     * @param enabled True to enable
     * @param transitionTime Time for transitions in seconds
     */
    void setSmoothTransitions(bool enabled, float transitionTime = 2.0f);
    
    // State access
    
    /**
     * Get full time of day state
     * @return Current state
     */
    const TimeOfDayState& getState() const { return m_state; }
    
    /**
     * Load state from file
     * @param filepath Path to state file
     * @return True if loaded successfully
     */
    bool loadState(const std::string& filepath);
    
    /**
     * Save state to file
     * @param filepath Path to save file
     * @return True if saved successfully
     */
    bool saveState(const std::string& filepath);

private:
    TimeOfDayState m_state;
    
    // Presets for each period
    struct PeriodPreset {
        SkyConfig sky;
        LightingConfig lighting;
    };
    PeriodPreset m_presets[8];  // One for each TimeOfDayPeriod
    
    // Transition state
    bool m_smoothTransitions;
    float m_transitionTime;
    float m_transitionProgress;
    PeriodPreset m_transitionFrom;
    PeriodPreset m_transitionTo;
    
    // Settings
    bool m_autoProgress;
    bool m_useCustomConfig;
    
    // Callbacks
    std::function<void(const TimeChangeEvent&)> m_timeChangeCallback;
    std::function<void(TimeOfDayPeriod)> m_periodChangeCallback;
    std::function<void()> m_sunriseCallback;
    std::function<void()> m_sunsetCallback;
    
    // Internal methods
    void updateTime(float deltaTime);
    void updatePeriod();
    void updateSunMoonPosition();
    void interpolateConfigs(const PeriodPreset& from, const PeriodPreset& to, float t);
    TimeOfDayPeriod calculatePeriod(float time) const;
    void initializePresets();
    void notifyTimeChange(float oldTime, float newTime);
    void notifyPeriodChange(TimeOfDayPeriod oldPeriod, TimeOfDayPeriod newPeriod);
};

// Helper functions
namespace TimeOfDayHelpers {
    /**
     * Get string name for time period
     * @param period Time period
     * @return String name
     */
    const char* getPeriodName(TimeOfDayPeriod period);
    
    /**
     * Get time range for a period
     * @param period Time period
     * @param outStart Output start time
     * @param outEnd Output end time
     */
    void getPeriodTimeRange(TimeOfDayPeriod period, float& outStart, float& outEnd);
    
    /**
     * Convert time to formatted string
     * @param hours Time in hours (0-24)
     * @return Formatted time string (e.g., "14:30")
     */
    std::string formatTime(float hours);
}

} // namespace Graphics
} // namespace JJM
