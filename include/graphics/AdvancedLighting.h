#pragma once

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include <vector>
#include <memory>

namespace JJM {
namespace Graphics {

enum class LightType {
    Point,
    Spot,
    Directional,
    Ambient
};

struct Light {
    LightType type;
    Math::Vector2D position;
    Math::Vector2D direction;
    Color color;
    float intensity;
    float radius;
    float innerConeAngle;
    float outerConeAngle;
    bool castsShadows;
    bool enabled;
    
    Light() : type(LightType::Point), position(0, 0), direction(0, -1),
              color(1.0f, 1.0f, 1.0f, 1.0f), intensity(1.0f), radius(100.0f),
              innerConeAngle(15.0f), outerConeAngle(30.0f),
              castsShadows(false), enabled(true) {}
};

class PointLight {
public:
    PointLight(const Math::Vector2D& position, const Color& color, float intensity, float radius);
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setColor(const Color& col) { color = col; }
    const Color& getColor() const { return color; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
    
    void setAttenuation(float constant, float linear, float quadratic);
    
    float calculateAttenuation(float distance) const;
    Color calculateLighting(const Math::Vector2D& point) const;

private:
    Math::Vector2D position;
    Color color;
    float intensity;
    float radius;
    
    float attenuationConstant;
    float attenuationLinear;
    float attenuationQuadratic;
};

class SpotLight {
public:
    SpotLight(const Math::Vector2D& position, const Math::Vector2D& direction,
              const Color& color, float intensity, float radius,
              float innerAngle, float outerAngle);
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setDirection(const Math::Vector2D& dir);
    const Math::Vector2D& getDirection() const { return direction; }
    
    void setColor(const Color& col) { color = col; }
    const Color& getColor() const { return color; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
    
    void setConeAngles(float inner, float outer);
    
    float calculateSpotEffect(const Math::Vector2D& point) const;
    Color calculateLighting(const Math::Vector2D& point) const;

private:
    Math::Vector2D position;
    Math::Vector2D direction;
    Color color;
    float intensity;
    float radius;
    float innerConeAngle;
    float outerConeAngle;
    
    float cosInnerAngle;
    float cosOuterAngle;
};

class DirectionalLight {
public:
    DirectionalLight(const Math::Vector2D& direction, const Color& color, float intensity);
    
    void setDirection(const Math::Vector2D& dir);
    const Math::Vector2D& getDirection() const { return direction; }
    
    void setColor(const Color& col) { color = col; }
    const Color& getColor() const { return color; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    Color calculateLighting() const;

private:
    Math::Vector2D direction;
    Color color;
    float intensity;
};

class AmbientLight {
public:
    AmbientLight(const Color& color, float intensity);
    
    void setColor(const Color& col) { color = col; }
    const Color& getColor() const { return color; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    Color calculateLighting() const;

private:
    Color color;
    float intensity;
};

class LightingSystem {
public:
    LightingSystem();
    ~LightingSystem();
    
    void addLight(const Light& light);
    void removeLight(size_t index);
    void clearLights();
    
    Light* getLight(size_t index);
    size_t getLightCount() const { return lights.size(); }
    
    void setAmbientColor(const Color& color);
    const Color& getAmbientColor() const { return ambientColor; }
    
    void setAmbientIntensity(float intensity);
    float getAmbientIntensity() const { return ambientIntensity; }
    
    Color calculateLighting(const Math::Vector2D& point, const Math::Vector2D& normal) const;
    
    void update(float deltaTime);
    void render();

private:
    std::vector<Light> lights;
    Color ambientColor;
    float ambientIntensity;
    
    Color calculatePointLight(const Light& light, const Math::Vector2D& point) const;
    Color calculateSpotLight(const Light& light, const Math::Vector2D& point) const;
    Color calculateDirectionalLight(const Light& light) const;
};

} // namespace Graphics
} // namespace JJM
