#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include <vector>
#include <functional>

namespace JJM {
namespace Particles {

struct Particle {
    Math::Vector2D position;
    Math::Vector2D velocity;
    Graphics::Color color;
    float lifetime;
    float maxLifetime;
    float size;
    float rotation;
    float rotationSpeed;
    bool active;
    
    Particle();
    void update(float deltaTime);
    bool isAlive() const;
};

class ParticleEmitter {
private:
    std::vector<Particle> particles;
    Math::Vector2D position;
    int maxParticles;
    float emissionRate;
    float emissionTimer;
    bool active;
    bool burst;
    
    // Emission properties
    float minLifetime, maxLifetime;
    float minSpeed, maxSpeed;
    float minSize, maxSize;
    float minAngle, maxAngle;
    Graphics::Color startColor, endColor;
    Math::Vector2D gravity;
    
public:
    ParticleEmitter(const Math::Vector2D& pos, int maxParticles = 100);
    
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);
    
    void emit(int count = 1);
    void emitBurst(int count);
    void start();
    void stop();
    void reset();
    
    // Setters for emission properties
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setEmissionRate(float rate) { emissionRate = rate; }
    void setLifetime(float min, float max);
    void setSpeed(float min, float max);
    void setSize(float min, float max);
    void setAngleRange(float min, float max);
    void setColorRange(const Graphics::Color& start, const Graphics::Color& end);
    void setGravity(const Math::Vector2D& g) { gravity = g; }
    
    // Getters
    bool isActive() const { return active; }
    int getActiveParticleCount() const;
    const Math::Vector2D& getPosition() const { return position; }
    
private:
    void createParticle();
    float randomFloat(float min, float max);
};

class ParticleSystem {
private:
    std::vector<ParticleEmitter*> emitters;
    
public:
    ParticleSystem();
    ~ParticleSystem();
    
    ParticleEmitter* createEmitter(const Math::Vector2D& position, int maxParticles = 100);
    void removeEmitter(ParticleEmitter* emitter);
    void removeAllEmitters();
    
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);
    
    int getEmitterCount() const { return static_cast<int>(emitters.size()); }
};

} // namespace Particles
} // namespace JJM

#endif // PARTICLE_SYSTEM_H
