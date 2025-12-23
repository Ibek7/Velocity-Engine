#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

// Wind system for environmental effects
namespace Engine {

struct WindZone {
    float position[3];
    float size[3];
    float direction[3];  // Normalized direction vector
    float strength;
    float turbulence;
    bool isGlobal;
};

class WindSystem {
public:
    using WindChangeCallback = std::function<void(float direction, float strength)>;

    static WindSystem& getInstance();

    // Global wind
    void setGlobalWind(float directionDegrees, float strength);
    void setGlobalWindDirection(float degrees);
    void setGlobalWindStrength(float strength);
    void getGlobalWind(float& directionDegrees, float& strength) const;
    
    // Wind zones
    void addWindZone(const WindZone& zone);
    void removeWindZone(float x, float y, float z);
    void clearWindZones();
    
    // Query wind at position
    void getWindAt(float x, float y, float z, float& directionX, float& directionY, float& directionZ, float& strength) const;
    float getWindStrengthAt(float x, float y, float z) const;
    
    // Dynamic wind
    void update(float deltaTime);
    void setDynamicWind(bool enable) { m_dynamicEnabled = enable; }
    void setWindVariation(float amount) { m_windVariation = amount; }
    void setWindChangeSpeed(float speed) { m_windChangeSpeed = speed; }
    
    // Gusts
    void createGust(float x, float y, float z, float radius, float strength, float duration);
    
    // Effects
    void setAffectParticles(bool affect) { m_affectParticles = affect; }
    void setAffectTrees(bool affect) { m_affectTrees = affect; }
    void setAffectCloth(bool affect) { m_affectCloth = affect; }
    
    // Events
    void onWindChange(WindChangeCallback callback);

private:
    WindSystem();
    WindSystem(const WindSystem&) = delete;
    WindSystem& operator=(const WindSystem&) = delete;

    void updateDynamicWind(float deltaTime);
    void updateGusts(float deltaTime);
    bool isPointInZone(const WindZone& zone, float x, float y, float z) const;

    float m_globalDirection;  // degrees
    float m_globalStrength;
    
    float m_targetDirection;
    float m_targetStrength;
    
    bool m_dynamicEnabled;
    float m_windVariation;
    float m_windChangeSpeed;
    float m_windChangeTimer;
    
    std::vector<WindZone> m_zones;
    
    struct Gust {
        float position[3];
        float radius;
        float strength;
        float duration;
        float elapsed;
    };
    std::vector<Gust> m_gusts;
    
    bool m_affectParticles;
    bool m_affectTrees;
    bool m_affectCloth;
    
    std::vector<WindChangeCallback> m_callbacks;
};

} // namespace Engine
