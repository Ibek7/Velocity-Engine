#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Weather system with dynamic conditions
namespace Engine {

enum class WeatherType {
    Clear,
    Cloudy,
    Rainy,
    Stormy,
    Snowy,
    Foggy,
    Windy
};

struct WeatherCondition {
    WeatherType type;
    float intensity;        // 0.0 - 1.0
    float windSpeed;
    float windDirection;    // Degrees
    float precipitation;    // 0.0 - 1.0
    float temperature;      // Celsius
    float humidity;         // 0.0 - 1.0
    float visibility;       // 0.0 - 1.0
};

struct WeatherTransition {
    WeatherType fromType;
    WeatherType toType;
    float duration;
    float elapsed;
};

class WeatherSystem {
public:
    static WeatherSystem& getInstance();

    // Main update
    void update(float deltaTime);
    
    // Weather control
    void setWeather(WeatherType type, float transitionTime = 5.0f);
    void setWeatherImmediate(WeatherType type);
    WeatherType getCurrentWeather() const { return m_currentCondition.type; }
    const WeatherCondition& getCurrentCondition() const { return m_currentCondition; }
    
    // Properties
    void setIntensity(float intensity);
    void setWindSpeed(float speed);
    void setWindDirection(float degrees);
    void setTemperature(float celsius);
    
    // Time of day integration
    void setTimeOfDay(float hours); // 0-24
    float getTimeOfDay() const { return m_timeOfDay; }
    void setDayNightCycleSpeed(float speed) { m_dayNightSpeed = speed; }
    
    // Queries
    bool isRaining() const;
    bool isStormy() const;
    bool isSnowing() const;
    bool isClear() const;
    float getLightingMultiplier() const;
    float getAmbientLightLevel() const;
    
    // Effects
    void enableDynamicSkybox(bool enable) { m_dynamicSkybox = enable; }
    void enableParticleEffects(bool enable) { m_particleEffects = enable; }
    void enableSoundEffects(bool enable) { m_soundEffects = enable; }
    
    // Random weather
    void enableRandomWeather(bool enable) { m_randomWeather = enable; }
    void setRandomWeatherInterval(float minSeconds, float maxSeconds);
    
    // Callbacks
    void onWeatherChanged(const std::function<void(WeatherType)>& callback);

private:
    WeatherSystem();
    WeatherSystem(const WeatherSystem&) = delete;
    WeatherSystem& operator=(const WeatherSystem&) = delete;

    void updateTransition(float deltaTime);
    void updateDayNightCycle(float deltaTime);
    void updateRandomWeather(float deltaTime);
    void applyWeatherEffects();
    void updateAmbientLight();
    void transitionToWeather(WeatherType type, float duration);
    WeatherCondition createConditionForType(WeatherType type);
    WeatherCondition interpolateConditions(const WeatherCondition& from, 
                                          const WeatherCondition& to, 
                                          float t);

    WeatherCondition m_currentCondition;
    WeatherCondition m_targetCondition;
    WeatherTransition m_activeTransition;
    bool m_isTransitioning;
    
    float m_timeOfDay;              // 0-24 hours
    float m_dayNightSpeed;          // Time scale
    
    bool m_randomWeather;
    float m_randomWeatherTimer;
    float m_minWeatherDuration;
    float m_maxWeatherDuration;
    
    bool m_dynamicSkybox;
    bool m_particleEffects;
    bool m_soundEffects;
    
    std::function<void(WeatherType)> m_onWeatherChanged;
};

} // namespace Engine
