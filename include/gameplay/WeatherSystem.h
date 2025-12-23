#pragma once

#include <string>
#include <vector>
#include <functional>

// Weather system for dynamic environment conditions
namespace Engine {

enum class WeatherType {
    Clear,
    Cloudy,
    Rainy,
    Stormy,
    Snowy,
    Foggy
};

struct WeatherConditions {
    WeatherType type;
    float intensity;          // 0.0 to 1.0
    float windSpeed;          // m/s
    float windDirection;      // degrees
    float temperature;        // celsius
    float humidity;           // 0.0 to 1.0
    float visibility;         // meters
    bool lightning;
};

class WeatherSystem {
public:
    using WeatherChangeCallback = std::function<void(const WeatherConditions&)>;

    static WeatherSystem& getInstance();

    // Weather control
    void setWeather(WeatherType type, float transitionTime = 5.0f);
    void setRandomWeather();
    WeatherConditions getCurrentWeather() const { return m_current; }
    
    // Update
    void update(float deltaTime);
    
    // Configuration
    void setIntensity(float intensity);
    void setWindSpeed(float speed);
    void setWindDirection(float degrees);
    void enableDynamicWeather(bool enable) { m_dynamicEnabled = enable; }
    void setWeatherChangeInterval(float seconds) { m_changeInterval = seconds; }
    
    // Events
    void onWeatherChange(WeatherChangeCallback callback);
    
    // Effects
    float getRainAmount() const;
    float getSnowAmount() const;
    float getFogAmount() const;
    bool shouldSpawnLightning() const;

private:
    WeatherSystem();
    WeatherSystem(const WeatherSystem&) = delete;
    WeatherSystem& operator=(const WeatherSystem&) = delete;

    void transitionWeather(float deltaTime);
    void updateDynamicWeather(float deltaTime);
    void triggerLightning();

    WeatherConditions m_current;
    WeatherConditions m_target;
    float m_transitionTime;
    float m_transitionProgress;
    
    bool m_dynamicEnabled;
    float m_changeInterval;
    float m_timeSinceLastChange;
    
    float m_lightningTimer;
    float m_lightningInterval;
    
    std::vector<WeatherChangeCallback> m_callbacks;
};

} // namespace Engine
