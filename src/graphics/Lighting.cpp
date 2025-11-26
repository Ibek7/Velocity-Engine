#include "graphics/Lighting.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// Light base class
Light::Light(LightType type, const Math::Vector2D& pos, const Color& col, float intensity)
    : type(type), position(pos), color(col), intensity(intensity), enabled(true) {
}

// PointLight
PointLight::PointLight(const Math::Vector2D& pos, const Color& col, float radius, float intensity)
    : Light(LightType::POINT, pos, col, intensity), radius(radius), falloff(1.0f) {
}

void PointLight::render(Renderer* renderer, SDL_Texture* lightMap) {
    if (!enabled || !renderer || !lightMap) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), lightMap);
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_ADD);
    
    int segments = 32;
    float angleStep = (2.0f * M_PI) / segments;
    
    for (int i = 0; i < segments; i++) {
        float currentAngle = i * angleStep;
        float nextAngle = (i + 1) * angleStep;
        
        // Draw light gradient from center to edge
        int steps = 20;
        for (int j = 0; j < steps; j++) {
            float t1 = static_cast<float>(j) / steps;
            float t2 = static_cast<float>(j + 1) / steps;
            
            float r1 = radius * t1;
            float r2 = radius * t2;
            
            float alpha = intensity * (1.0f - std::pow(t1, falloff));
            
            SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
                color.r, color.g, color.b,
                static_cast<Uint8>(alpha * 255));
            
            // Draw quad segment
            float x1 = position.x + r1 * std::cos(currentAngle);
            float y1 = position.y + r1 * std::sin(currentAngle);
            float x2 = position.x + r2 * std::cos(currentAngle);
            float y2 = position.y + r2 * std::sin(currentAngle);
            float x3 = position.x + r2 * std::cos(nextAngle);
            float y3 = position.y + r2 * std::sin(nextAngle);
            float x4 = position.x + r1 * std::cos(nextAngle);
            float y4 = position.y + r1 * std::sin(nextAngle);
            
            SDL_RenderDrawLine(renderer->getSDLRenderer(), x1, y1, x2, y2);
        }
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// DirectionalLight
DirectionalLight::DirectionalLight(const Math::Vector2D& dir, const Color& col, float intensity)
    : Light(LightType::DIRECTIONAL, Math::Vector2D(0, 0), col, intensity),
      direction(dir.normalized()) {
}

void DirectionalLight::render(Renderer* renderer, SDL_Texture* lightMap) {
    if (!enabled || !renderer || !lightMap) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), lightMap);
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_ADD);
    
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
        static_cast<Uint8>(color.r * intensity),
        static_cast<Uint8>(color.g * intensity),
        static_cast<Uint8>(color.b * intensity),
        static_cast<Uint8>(255 * intensity));
    
    SDL_RenderFillRect(renderer->getSDLRenderer(), nullptr);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// SpotLight
SpotLight::SpotLight(const Math::Vector2D& pos, const Math::Vector2D& dir,
                     const Color& col, float angle, float radius, float intensity)
    : Light(LightType::SPOT, pos, col, intensity),
      direction(dir.normalized()), angle(angle), radius(radius) {
}

void SpotLight::render(Renderer* renderer, SDL_Texture* lightMap) {
    if (!enabled || !renderer || !lightMap) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), lightMap);
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_ADD);
    
    float halfAngle = angle * 0.5f;
    int segments = 32;
    
    for (int i = 0; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        float currentAngle = -halfAngle + (halfAngle * 2.0f * t);
        
        float dirAngle = std::atan2(direction.y, direction.x);
        float finalAngle = dirAngle + currentAngle;
        
        float alpha = intensity * (1.0f - std::abs(currentAngle) / halfAngle);
        
        SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
            color.r, color.g, color.b,
            static_cast<Uint8>(alpha * 255));
        
        float x = position.x + radius * std::cos(finalAngle);
        float y = position.y + radius * std::sin(finalAngle);
        
        SDL_RenderDrawLine(renderer->getSDLRenderer(),
            position.x, position.y, x, y);
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// LightingSystem
LightingSystem::LightingSystem(int width, int height)
    : width(width), height(height), lightMap(nullptr),
      ambientLight(50, 50, 50, 255) {
}

LightingSystem::~LightingSystem() {
    clearLights();
    destroyLightMap();
}

void LightingSystem::addLight(Light* light) {
    if (light) {
        lights.push_back(light);
    }
}

void LightingSystem::removeLight(Light* light) {
    auto it = std::find(lights.begin(), lights.end(), light);
    if (it != lights.end()) {
        lights.erase(it);
    }
}

void LightingSystem::clearLights() {
    for (auto* light : lights) {
        delete light;
    }
    lights.clear();
}

void LightingSystem::setAmbientLight(const Color& color) {
    ambientLight = color;
}

void LightingSystem::render(Renderer* renderer) {
    if (!renderer) return;
    
    if (!lightMap) {
        createLightMap(renderer);
    }
    
    if (!lightMap) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), lightMap);
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
        ambientLight.r, ambientLight.g, ambientLight.b, ambientLight.a);
    SDL_RenderClear(renderer->getSDLRenderer());
    
    for (auto* light : lights) {
        if (light && light->isEnabled()) {
            light->render(renderer, lightMap);
        }
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

void LightingSystem::apply(Renderer* renderer) {
    if (!renderer || !lightMap) return;
    
    SDL_SetTextureBlendMode(lightMap, SDL_BLENDMODE_MOD);
    SDL_RenderCopy(renderer->getSDLRenderer(), lightMap, nullptr, nullptr);
}

void LightingSystem::createLightMap(Renderer* renderer) {
    if (!renderer) return;
    
    lightMap = SDL_CreateTexture(renderer->getSDLRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height);
    
    if (lightMap) {
        SDL_SetTextureBlendMode(lightMap, SDL_BLENDMODE_MOD);
    }
}

void LightingSystem::destroyLightMap() {
    if (lightMap) {
        SDL_DestroyTexture(lightMap);
        lightMap = nullptr;
    }
}

} // namespace Graphics
} // namespace JJM
