#include "graphics/AdvancedLighting.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// PointLight implementation
PointLight::PointLight(const Math::Vector2D& position, const Color& color, 
                       float intensity, float radius)
    : position(position), color(color), intensity(intensity), radius(radius),
      attenuationConstant(1.0f), attenuationLinear(0.09f), attenuationQuadratic(0.032f) {}

void PointLight::setAttenuation(float constant, float linear, float quadratic) {
    attenuationConstant = constant;
    attenuationLinear = linear;
    attenuationQuadratic = quadratic;
}

float PointLight::calculateAttenuation(float distance) const {
    if (distance > radius) return 0.0f;
    
    return 1.0f / (attenuationConstant + attenuationLinear * distance + 
                   attenuationQuadratic * distance * distance);
}

Color PointLight::calculateLighting(const Math::Vector2D& point) const {
    Math::Vector2D diff = point - position;
    float distance = diff.magnitude();
    
    if (distance > radius) {
        return Color(0, 0, 0, 0);
    }
    
    float attenuation = calculateAttenuation(distance);
    
    return Color(
        static_cast<uint8_t>(color.r * attenuation),
        static_cast<uint8_t>(color.g * attenuation),
        static_cast<uint8_t>(color.b * attenuation),
        color.a
    );
}

// SpotLight implementation
SpotLight::SpotLight(const Math::Vector2D& position, const Math::Vector2D& direction,
                     const Color& color, float intensity, float radius,
                     float innerAngle, float outerAngle)
    : position(position), direction(direction), color(color),
      intensity(intensity), radius(radius),
      innerConeAngle(innerAngle), outerConeAngle(outerAngle) {
    
    cosInnerAngle = std::cos(innerAngle * M_PI / 180.0f);
    cosOuterAngle = std::cos(outerAngle * M_PI / 180.0f);
}

void SpotLight::setDirection(const Math::Vector2D& dir) {
    direction = dir;
    direction.normalize();
}

void SpotLight::setConeAngles(float inner, float outer) {
    innerConeAngle = inner;
    outerConeAngle = outer;
    cosInnerAngle = std::cos(inner * M_PI / 180.0f);
    cosOuterAngle = std::cos(outer * M_PI / 180.0f);
}

float SpotLight::calculateSpotEffect(const Math::Vector2D& point) const {
    Math::Vector2D lightToPoint = point - position;
    float distance = lightToPoint.magnitude();
    
    if (distance > radius) return 0.0f;
    
    lightToPoint.normalize();
    
    float cosAngle = lightToPoint.dot(direction);
    
    if (cosAngle < cosOuterAngle) {
        return 0.0f;
    }
    
    if (cosAngle > cosInnerAngle) {
        return 1.0f;
    }
    
    float epsilon = cosInnerAngle - cosOuterAngle;
    return (cosAngle - cosOuterAngle) / epsilon;
}

Color SpotLight::calculateLighting(const Math::Vector2D& point) const {
    Math::Vector2D diff = point - position;
    float distance = diff.magnitude();
    
    if (distance > radius) {
        return Color(0, 0, 0, 0);
    }
    
    float spotEffect = calculateSpotEffect(point);
    
    if (spotEffect == 0.0f) {
        return Color(0, 0, 0, 0);
    }
    
    float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * distance * distance);
    
    return Color(
        static_cast<uint8_t>(color.r * attenuation * spotEffect),
        static_cast<uint8_t>(color.g * attenuation * spotEffect),
        static_cast<uint8_t>(color.b * attenuation * spotEffect),
        color.a
    );
}

// DirectionalLight implementation
DirectionalLight::DirectionalLight(const Math::Vector2D& direction, 
                                   const Color& color, float intensity)
    : direction(direction), color(color), intensity(intensity) {
    this->direction.normalize();
}

void DirectionalLight::setDirection(const Math::Vector2D& dir) {
    direction = dir;
    direction.normalize();
}

Color DirectionalLight::calculateLighting() const {
    return Color(
        static_cast<uint8_t>(color.r * intensity),
        static_cast<uint8_t>(color.g * intensity),
        static_cast<uint8_t>(color.b * intensity),
        color.a
    );
}

// AmbientLight implementation
AmbientLight::AmbientLight(const Color& color, float intensity)
    : color(color), intensity(intensity) {}

Color AmbientLight::calculateLighting() const {
    return Color(
        static_cast<uint8_t>(color.r * intensity),
        static_cast<uint8_t>(color.g * intensity),
        static_cast<uint8_t>(color.b * intensity),
        color.a
    );
}

// LightingSystem implementation
LightingSystem::LightingSystem()
    : ambientColor(51, 51, 51, 255), ambientIntensity(1.0f) {}

LightingSystem::~LightingSystem() {}

void LightingSystem::addLight(const Light& light) {
    lights.push_back(light);
}

void LightingSystem::removeLight(size_t index) {
    if (index < lights.size()) {
        lights.erase(lights.begin() + index);
    }
}

void LightingSystem::clearLights() {
    lights.clear();
}

Light* LightingSystem::getLight(size_t index) {
    if (index < lights.size()) {
        return &lights[index];
    }
    return nullptr;
}

void LightingSystem::setAmbientColor(const Color& color) {
    ambientColor = color;
}

void LightingSystem::setAmbientIntensity(float intensity) {
    ambientIntensity = intensity;
}

Color LightingSystem::calculateLighting(const Math::Vector2D& point, 
                                        const Math::Vector2D& normal) const {
    Color totalLight = Color(
        static_cast<uint8_t>(ambientColor.r * ambientIntensity),
        static_cast<uint8_t>(ambientColor.g * ambientIntensity),
        static_cast<uint8_t>(ambientColor.b * ambientIntensity),
        255
    );
    
    for (const auto& light : lights) {
        if (!light.enabled) continue;
        
        Color lightContribution;
        
        switch (light.type) {
            case LightType::Point:
                lightContribution = calculatePointLight(light, point);
                break;
                
            case LightType::Spot:
                lightContribution = calculateSpotLight(light, point);
                break;
                
            case LightType::Directional:
                lightContribution = calculateDirectionalLight(light);
                break;
                
            case LightType::Ambient:
                lightContribution = Color(
                    static_cast<uint8_t>(light.color.r * light.intensity),
                    static_cast<uint8_t>(light.color.g * light.intensity),
                    static_cast<uint8_t>(light.color.b * light.intensity),
                    light.color.a
                );
                break;
        }
        
        totalLight.r = std::min<uint8_t>(totalLight.r + lightContribution.r, 255);
        totalLight.g = std::min<uint8_t>(totalLight.g + lightContribution.g, 255);
        totalLight.b = std::min<uint8_t>(totalLight.b + lightContribution.b, 255);
    }
    
    return totalLight;
}

void LightingSystem::update(float deltaTime) {
    // Update logic for animated lights
}

void LightingSystem::render() {
    // Rendering handled by graphics system
}

Color LightingSystem::calculatePointLight(const Light& light, 
                                          const Math::Vector2D& point) const {
    Math::Vector2D diff = point - light.position;
    float distance = diff.magnitude();
    
    if (distance > light.radius) {
        return Color(0, 0, 0, 0);
    }
    
    float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * distance * distance);
    
    return Color(
        static_cast<uint8_t>(light.color.r * light.intensity * attenuation),
        static_cast<uint8_t>(light.color.g * light.intensity * attenuation),
        static_cast<uint8_t>(light.color.b * light.intensity * attenuation),
        light.color.a
    );
}

Color LightingSystem::calculateSpotLight(const Light& light, 
                                         const Math::Vector2D& point) const {
    Math::Vector2D diff = point - light.position;
    float distance = diff.magnitude();
    
    if (distance > light.radius) {
        return Color(0, 0, 0, 0);
    }
    
    Math::Vector2D lightToPoint = diff;
    lightToPoint.normalize();
    
    float cosAngle = lightToPoint.dot(light.direction);
    float cosOuter = std::cos(light.outerConeAngle * M_PI / 180.0f);
    float cosInner = std::cos(light.innerConeAngle * M_PI / 180.0f);
    
    if (cosAngle < cosOuter) {
        return Color(0, 0, 0, 0);
    }
    
    float spotEffect = 1.0f;
    if (cosAngle < cosInner) {
        float epsilon = cosInner - cosOuter;
        spotEffect = (cosAngle - cosOuter) / epsilon;
    }
    
    float attenuation = 1.0f / (1.0f + 0.09f * distance + 0.032f * distance * distance);
    
    return Color(
        static_cast<uint8_t>(light.color.r * light.intensity * attenuation * spotEffect),
        static_cast<uint8_t>(light.color.g * light.intensity * attenuation * spotEffect),
        static_cast<uint8_t>(light.color.b * light.intensity * attenuation * spotEffect),
        light.color.a
    );
}

Color LightingSystem::calculateDirectionalLight(const Light& light) const {
    return Color(
        light.color.r * light.intensity,
        light.color.g * light.intensity,
        light.color.b * light.intensity,
        light.color.a
    );
}

} // namespace Graphics
} // namespace JJM
