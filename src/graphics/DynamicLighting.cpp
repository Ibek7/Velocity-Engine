#include "graphics/DynamicLighting.h"
#include <cmath>

namespace Engine {

DynamicLighting::DynamicLighting()
    : m_nextLightId(0)
    , m_ambientIntensity(0.2f)
    , m_shadowQuality(1)
    , m_maxShadowCasters(4)
{
    m_ambientColor[0] = 0.3f;
    m_ambientColor[1] = 0.3f;
    m_ambientColor[2] = 0.4f;
}

DynamicLighting& DynamicLighting::getInstance() {
    static DynamicLighting instance;
    return instance;
}

int DynamicLighting::addLight(LightType type, float x, float y, float z) {
    Light light;
    light.id = m_nextLightId++;
    light.type = type;
    light.position[0] = x;
    light.position[1] = y;
    light.position[2] = z;
    light.direction[0] = 0.0f;
    light.direction[1] = -1.0f;
    light.direction[2] = 0.0f;
    light.color[0] = 1.0f;
    light.color[1] = 1.0f;
    light.color[2] = 1.0f;
    light.intensity = 1.0f;
    light.radius = 10.0f;
    light.spotAngle = 45.0f;
    light.castsShadows = false;
    light.enabled = true;
    
    m_lights[light.id] = light;
    return light.id;
}

void DynamicLighting::removeLight(int lightId) {
    m_lights.erase(lightId);
}

void DynamicLighting::clearLights() {
    m_lights.clear();
}

void DynamicLighting::setLightPosition(int lightId, float x, float y, float z) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.position[0] = x;
        it->second.position[1] = y;
        it->second.position[2] = z;
    }
}

void DynamicLighting::setLightDirection(int lightId, float dx, float dy, float dz) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        // Normalize
        float len = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (len > 0.001f) {
            it->second.direction[0] = dx / len;
            it->second.direction[1] = dy / len;
            it->second.direction[2] = dz / len;
        }
    }
}

void DynamicLighting::setLightColor(int lightId, float r, float g, float b) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.color[0] = r;
        it->second.color[1] = g;
        it->second.color[2] = b;
    }
}

void DynamicLighting::setLightIntensity(int lightId, float intensity) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.intensity = intensity;
    }
}

void DynamicLighting::setLightRadius(int lightId, float radius) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.radius = radius;
    }
}

void DynamicLighting::setLightSpotAngle(int lightId, float degrees) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.spotAngle = degrees;
    }
}

void DynamicLighting::setLightCastsShadows(int lightId, bool casts) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.castsShadows = casts;
    }
}

void DynamicLighting::setLightEnabled(int lightId, bool enabled) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        it->second.enabled = enabled;
    }
}

Light* DynamicLighting::getLight(int lightId) {
    auto it = m_lights.find(lightId);
    if (it != m_lights.end()) {
        return &it->second;
    }
    return nullptr;
}

void DynamicLighting::getLightsInRadius(float x, float y, float z, float radius, std::vector<Light*>& results) {
    results.clear();
    float radiusSq = radius * radius;
    
    for (auto& pair : m_lights) {
        if (!pair.second.enabled) continue;
        
        float dx = pair.second.position[0] - x;
        float dy = pair.second.position[1] - y;
        float dz = pair.second.position[2] - z;
        
        float distSq = dx*dx + dy*dy + dz*dz;
        if (distSq <= radiusSq) {
            results.push_back(&pair.second);
        }
    }
}

void DynamicLighting::update(float deltaTime) {
    // TODO: Update lighting state
    (void)deltaTime;
}

void DynamicLighting::applyLighting() {
    // TODO: Apply lights to rendering pipeline
}

void DynamicLighting::setAmbientLight(float r, float g, float b, float intensity) {
    m_ambientColor[0] = r;
    m_ambientColor[1] = g;
    m_ambientColor[2] = b;
    m_ambientIntensity = intensity;
}

void DynamicLighting::getAmbientLight(float& r, float& g, float& b, float& intensity) const {
    r = m_ambientColor[0];
    g = m_ambientColor[1];
    b = m_ambientColor[2];
    intensity = m_ambientIntensity;
}

} // namespace Engine
