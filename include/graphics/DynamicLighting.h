#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Dynamic lighting system with real-time shadows
namespace Engine {

enum class LightType {
    Point,
    Directional,
    Spot,
    Area
};

struct Light {
    int id;
    LightType type;
    float position[3];
    float direction[3];
    float color[3];
    float intensity;
    float radius;
    float spotAngle;
    bool castsShadows;
    bool enabled;
};

class DynamicLighting {
public:
    static DynamicLighting& getInstance();

    // Light management
    int addLight(LightType type, float x, float y, float z);
    void removeLight(int lightId);
    void clearLights();
    
    // Light properties
    void setLightPosition(int lightId, float x, float y, float z);
    void setLightDirection(int lightId, float dx, float dy, float dz);
    void setLightColor(int lightId, float r, float g, float b);
    void setLightIntensity(int lightId, float intensity);
    void setLightRadius(int lightId, float radius);
    void setLightSpotAngle(int lightId, float degrees);
    void setLightCastsShadows(int lightId, bool casts);
    void setLightEnabled(int lightId, bool enabled);
    
    // Query
    Light* getLight(int lightId);
    void getLightsInRadius(float x, float y, float z, float radius, std::vector<Light*>& results);
    int getLightCount() const { return m_lights.size(); }
    
    // Rendering
    void update(float deltaTime);
    void applyLighting();
    
    // Global settings
    void setAmbientLight(float r, float g, float b, float intensity);
    void getAmbientLight(float& r, float& g, float& b, float& intensity) const;
    void setShadowQuality(int quality) { m_shadowQuality = quality; }
    void setMaxShadowCasters(int max) { m_maxShadowCasters = max; }

private:
    DynamicLighting();
    DynamicLighting(const DynamicLighting&) = delete;
    DynamicLighting& operator=(const DynamicLighting&) = delete;

    std::unordered_map<int, Light> m_lights;
    int m_nextLightId;
    
    float m_ambientColor[3];
    float m_ambientIntensity;
    int m_shadowQuality;
    int m_maxShadowCasters;
};

} // namespace Engine
