#include "particles/ParticleSystem.h"
#include <cmath>
#include <random>
#include <algorithm>

namespace JJM {
namespace Particles {

// Random number generator
static std::random_device rd;
static std::mt19937 gen(rd());

// Particle implementation
Particle::Particle() 
    : position(0, 0), velocity(0, 0), color(Graphics::Color::White()),
      lifetime(0), maxLifetime(1.0f), size(5.0f), rotation(0), 
      rotationSpeed(0), active(false) {}

void Particle::update(float deltaTime) {
    if (!active) return;
    
    lifetime -= deltaTime;
    if (lifetime <= 0) {
        active = false;
        return;
    }
    
    position += velocity * deltaTime;
    rotation += rotationSpeed * deltaTime;
    
    // Fade out
    float lifeRatio = lifetime / maxLifetime;
    color.a = static_cast<uint8_t>(255 * lifeRatio);
}

bool Particle::isAlive() const {
    return active && lifetime > 0;
}

// ParticleEmitter implementation
ParticleEmitter::ParticleEmitter(const Math::Vector2D& pos, int maxParticles)
    : position(pos), maxParticles(maxParticles), emissionRate(10.0f),
      emissionTimer(0), active(false), burst(false),
      minLifetime(1.0f), maxLifetime(3.0f),
      minSpeed(50.0f), maxSpeed(150.0f),
      minSize(2.0f), maxSize(8.0f),
      minAngle(0), maxAngle(2.0f * M_PI),
      startColor(Graphics::Color::White()), endColor(Graphics::Color::White()),
      gravity(0, 0) {
    
    particles.resize(maxParticles);
}

void ParticleEmitter::update(float deltaTime) {
    // Update emission
    if (active && !burst) {
        emissionTimer += deltaTime;
        float emissionInterval = 1.0f / emissionRate;
        
        while (emissionTimer >= emissionInterval) {
            emit(1);
            emissionTimer -= emissionInterval;
        }
    }
    
    // Update all particles
    for (auto& particle : particles) {
        if (particle.active) {
            particle.velocity += gravity * deltaTime;
            particle.update(deltaTime);
        }
    }
}

void ParticleEmitter::render(Graphics::Renderer* renderer) {
    for (const auto& particle : particles) {
        if (particle.active) {
            Math::Vector2D size(particle.size, particle.size);
            renderer->drawRect(particle.position - size * 0.5f, size, particle.color, true);
        }
    }
}

void ParticleEmitter::emit(int count) {
    for (int i = 0; i < count; ++i) {
        createParticle();
    }
}

void ParticleEmitter::emitBurst(int count) {
    for (int i = 0; i < count; ++i) {
        createParticle();
    }
}

void ParticleEmitter::start() {
    active = true;
    burst = false;
    emissionTimer = 0;
}

void ParticleEmitter::stop() {
    active = false;
}

void ParticleEmitter::reset() {
    for (auto& particle : particles) {
        particle.active = false;
    }
    emissionTimer = 0;
}

void ParticleEmitter::setLifetime(float min, float max) {
    minLifetime = min;
    maxLifetime = max;
}

void ParticleEmitter::setSpeed(float min, float max) {
    minSpeed = min;
    maxSpeed = max;
}

void ParticleEmitter::setSize(float min, float max) {
    minSize = min;
    maxSize = max;
}

void ParticleEmitter::setAngleRange(float min, float max) {
    minAngle = min;
    maxAngle = max;
}

void ParticleEmitter::setColorRange(const Graphics::Color& start, const Graphics::Color& end) {
    startColor = start;
    endColor = end;
}

int ParticleEmitter::getActiveParticleCount() const {
    int count = 0;
    for (const auto& particle : particles) {
        if (particle.active) count++;
    }
    return count;
}

void ParticleEmitter::createParticle() {
    // Find inactive particle
    for (auto& particle : particles) {
        if (!particle.active) {
            particle.position = position;
            
            float angle = randomFloat(minAngle, maxAngle);
            float speed = randomFloat(minSpeed, maxSpeed);
            particle.velocity = Math::Vector2D(std::cos(angle), std::sin(angle)) * speed;
            
            particle.lifetime = randomFloat(minLifetime, maxLifetime);
            particle.maxLifetime = particle.lifetime;
            particle.size = randomFloat(minSize, maxSize);
            particle.rotation = randomFloat(0, 2.0f * M_PI);
            particle.rotationSpeed = randomFloat(-2.0f, 2.0f);
            
            // Color interpolation
            float t = randomFloat(0, 1);
            particle.color = startColor.blend(endColor, t);
            
            particle.active = true;
            break;
        }
    }
}

float ParticleEmitter::randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// ParticleSystem implementation
ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {
    removeAllEmitters();
}

ParticleEmitter* ParticleSystem::createEmitter(const Math::Vector2D& position, int maxParticles) {
    ParticleEmitter* emitter = new ParticleEmitter(position, maxParticles);
    emitters.push_back(emitter);
    return emitter;
}

void ParticleSystem::removeEmitter(ParticleEmitter* emitter) {
    auto it = std::find(emitters.begin(), emitters.end(), emitter);
    if (it != emitters.end()) {
        delete *it;
        emitters.erase(it);
    }
}

void ParticleSystem::removeAllEmitters() {
    for (auto* emitter : emitters) {
        delete emitter;
    }
    emitters.clear();
}

void ParticleSystem::update(float deltaTime) {
    for (auto* emitter : emitters) {
        emitter->update(deltaTime);
    }
}

void ParticleSystem::render(Graphics::Renderer* renderer) {
    for (auto* emitter : emitters) {
        emitter->render(renderer);
    }
}

} // namespace Particles
} // namespace JJM
