#ifndef LIGHTING_H
#define LIGHTING_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include <vector>
#include <SDL.h>

namespace JJM {
namespace Graphics {

enum class LightType {
    POINT,
    DIRECTIONAL,
    SPOT
};

class Light {
protected:
    LightType type;
    Math::Vector2D position;
    Color color;
    float intensity;
    bool enabled;
    
public:
    Light(LightType type, const Math::Vector2D& pos, const Color& col, float intensity);
    virtual ~Light() = default;
    
    virtual void render(Renderer* renderer, SDL_Texture* lightMap) = 0;
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    Math::Vector2D getPosition() const { return position; }
    
    void setColor(const Color& col) { color = col; }
    Color getColor() const { return color; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }
    
    LightType getType() const { return type; }
};

class PointLight : public Light {
private:
    float radius;
    float falloff;
    
public:
    PointLight(const Math::Vector2D& pos, const Color& col, float radius, float intensity = 1.0f);
    
    void render(Renderer* renderer, SDL_Texture* lightMap) override;
    
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
    
    void setFalloff(float f) { falloff = f; }
    float getFalloff() const { return falloff; }
};

class DirectionalLight : public Light {
private:
    Math::Vector2D direction;
    
public:
    DirectionalLight(const Math::Vector2D& dir, const Color& col, float intensity = 1.0f);
    
    void render(Renderer* renderer, SDL_Texture* lightMap) override;
    
    void setDirection(const Math::Vector2D& dir) { direction = dir.normalized(); }
    Math::Vector2D getDirection() const { return direction; }
};

class SpotLight : public Light {
private:
    Math::Vector2D direction;
    float angle;
    float radius;
    
public:
    SpotLight(const Math::Vector2D& pos, const Math::Vector2D& dir,
             const Color& col, float angle, float radius, float intensity = 1.0f);
    
    void render(Renderer* renderer, SDL_Texture* lightMap) override;
    
    void setDirection(const Math::Vector2D& dir) { direction = dir.normalized(); }
    Math::Vector2D getDirection() const { return direction; }
    
    void setAngle(float a) { angle = a; }
    float getAngle() const { return angle; }
    
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
};

class LightingSystem {
private:
    std::vector<Light*> lights;
    Color ambientLight;
    SDL_Texture* lightMap;
    int width;
    int height;
    
public:
    LightingSystem(int width, int height);
    ~LightingSystem();
    
    void addLight(Light* light);
    void removeLight(Light* light);
    void clearLights();
    
    void setAmbientLight(const Color& color);
    Color getAmbientLight() const { return ambientLight; }
    
    void render(Renderer* renderer);
    void apply(Renderer* renderer);
    
    SDL_Texture* getLightMap() const { return lightMap; }
    
    int getLightCount() const { return static_cast<int>(lights.size()); }
    
private:
    void createLightMap(Renderer* renderer);
    void destroyLightMap();
};

} // namespace Graphics
} // namespace JJM

#endif // LIGHTING_H
