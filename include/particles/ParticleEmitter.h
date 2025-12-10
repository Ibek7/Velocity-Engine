#pragma once

#include "math/Vector2D.h"
#include <vector>
#include <memory>
#include <functional>

namespace JJM {
namespace Particles {

enum class EmitterShape {
    Point,
    Circle,
    Rectangle,
    Line,
    Cone
};

struct EmitterShapeConfig {
    EmitterShape type;
    float radius;
    float width;
    float height;
    float angle;
    Math::Vector2D direction;
    
    EmitterShapeConfig() 
        : type(EmitterShape::Point), radius(1.0f), 
          width(1.0f), height(1.0f), angle(0.0f), direction(1, 0) {}
};

struct Particle {
    Math::Vector2D position;
    Math::Vector2D velocity;
    Math::Vector2D acceleration;
    float rotation;
    float angularVelocity;
    float scale;
    float lifetime;
    float age;
    float color[4];
    bool active;
    
    Particle() : rotation(0), angularVelocity(0), scale(1), 
                 lifetime(1), age(0), active(false) {
        color[0] = 1.0f; color[1] = 1.0f; 
        color[2] = 1.0f; color[3] = 1.0f;
    }
};

class ParticleEmitter {
public:
    ParticleEmitter(size_t maxParticles = 1000);
    ~ParticleEmitter();
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    
    void setEmitterShape(const EmitterShapeConfig& config) { shapeConfig = config; }
    const EmitterShapeConfig& getEmitterShape() const { return shapeConfig; }
    
    void setEmissionRate(float rate) { emissionRate = rate; }
    void setParticleLifetime(float min, float max) { 
        minLifetime = min; maxLifetime = max; 
    }
    
    void setVelocity(const Math::Vector2D& min, const Math::Vector2D& max) {
        minVelocity = min; maxVelocity = max;
    }
    
    void setStartColor(float r, float g, float b, float a);
    void setEndColor(float r, float g, float b, float a);
    
    void setStartScale(float scale) { startScale = scale; }
    void setEndScale(float scale) { endScale = scale; }
    
    void emit(int count = 1);
    void update(float deltaTime);
    void render();
    
    void start() { isEmitting = true; }
    void stop() { isEmitting = false; }
    void clear();
    
    size_t getActiveParticleCount() const { return activeParticles; }
    bool isActive() const { return isEmitting; }

private:
    std::vector<Particle> particles;
    size_t maxParticles;
    size_t activeParticles;
    
    Math::Vector2D position;
    EmitterShapeConfig shapeConfig;
    
    float emissionRate;
    float emissionTimer;
    bool isEmitting;
    
    float minLifetime;
    float maxLifetime;
    
    Math::Vector2D minVelocity;
    Math::Vector2D maxVelocity;
    
    float startColor[4];
    float endColor[4];
    
    float startScale;
    float endScale;
    
    Math::Vector2D getEmissionPosition();
    Math::Vector2D getEmissionVelocity();
    
    void updateParticle(Particle& particle, float deltaTime);
};

class CircleEmitter {
public:
    CircleEmitter(float radius, bool emitFromEdge = false);
    
    Math::Vector2D getRandomPosition(const Math::Vector2D& center) const;
    Math::Vector2D getRandomDirection() const;

private:
    float radius;
    bool emitFromEdge;
};

class RectangleEmitter {
public:
    RectangleEmitter(float width, float height);
    
    Math::Vector2D getRandomPosition(const Math::Vector2D& center) const;

private:
    float width;
    float height;
};

class ConeEmitter {
public:
    ConeEmitter(float angle, const Math::Vector2D& direction);
    
    Math::Vector2D getRandomDirection() const;

private:
    float angle;
    Math::Vector2D direction;
};

} // namespace Particles
} // namespace JJM
