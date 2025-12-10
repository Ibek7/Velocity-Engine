#include "particles/ParticleEmitter.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace JJM {
namespace Particles {

static float randomFloat(float min, float max) {
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

ParticleEmitter::ParticleEmitter(size_t maxParticles)
    : maxParticles(maxParticles), activeParticles(0),
      position(0, 0), emissionRate(10.0f), emissionTimer(0.0f),
      isEmitting(false), minLifetime(1.0f), maxLifetime(2.0f),
      minVelocity(-10, -10), maxVelocity(10, 10),
      startScale(1.0f), endScale(0.5f) {
    
    particles.resize(maxParticles);
    
    startColor[0] = 1.0f; startColor[1] = 1.0f;
    startColor[2] = 1.0f; startColor[3] = 1.0f;
    
    endColor[0] = 1.0f; endColor[1] = 1.0f;
    endColor[2] = 1.0f; endColor[3] = 0.0f;
}

ParticleEmitter::~ParticleEmitter() {}

void ParticleEmitter::setStartColor(float r, float g, float b, float a) {
    startColor[0] = r; startColor[1] = g;
    startColor[2] = b; startColor[3] = a;
}

void ParticleEmitter::setEndColor(float r, float g, float b, float a) {
    endColor[0] = r; endColor[1] = g;
    endColor[2] = b; endColor[3] = a;
}

void ParticleEmitter::emit(int count) {
    for (int i = 0; i < count && activeParticles < maxParticles; ++i) {
        for (auto& particle : particles) {
            if (!particle.active) {
                particle.active = true;
                particle.position = getEmissionPosition();
                particle.velocity = getEmissionVelocity();
                particle.acceleration = Math::Vector2D(0, 0);
                particle.rotation = randomFloat(0, 2.0f * M_PI);
                particle.angularVelocity = randomFloat(-2.0f, 2.0f);
                particle.scale = startScale;
                particle.lifetime = randomFloat(minLifetime, maxLifetime);
                particle.age = 0.0f;
                
                for (int j = 0; j < 4; ++j) {
                    particle.color[j] = startColor[j];
                }
                
                ++activeParticles;
                break;
            }
        }
    }
}

void ParticleEmitter::update(float deltaTime) {
    if (isEmitting) {
        emissionTimer += deltaTime;
        float emissionInterval = 1.0f / emissionRate;
        
        while (emissionTimer >= emissionInterval) {
            emit(1);
            emissionTimer -= emissionInterval;
        }
    }
    
    for (auto& particle : particles) {
        if (particle.active) {
            updateParticle(particle, deltaTime);
        }
    }
}

void ParticleEmitter::render() {
    // Rendering handled by graphics system
}

void ParticleEmitter::clear() {
    for (auto& particle : particles) {
        particle.active = false;
    }
    activeParticles = 0;
}

Math::Vector2D ParticleEmitter::getEmissionPosition() {
    switch (shapeConfig.type) {
        case EmitterShape::Point:
            return position;
            
        case EmitterShape::Circle: {
            float angle = randomFloat(0, 2.0f * M_PI);
            float radius = randomFloat(0, shapeConfig.radius);
            return position + Math::Vector2D(
                std::cos(angle) * radius,
                std::sin(angle) * radius
            );
        }
        
        case EmitterShape::Rectangle: {
            float x = randomFloat(-shapeConfig.width * 0.5f, shapeConfig.width * 0.5f);
            float y = randomFloat(-shapeConfig.height * 0.5f, shapeConfig.height * 0.5f);
            return position + Math::Vector2D(x, y);
        }
        
        case EmitterShape::Line: {
            float t = randomFloat(0, 1);
            Math::Vector2D offset = shapeConfig.direction * shapeConfig.width * t;
            return position + offset;
        }
        
        case EmitterShape::Cone:
            return position;
    }
    
    return position;
}

Math::Vector2D ParticleEmitter::getEmissionVelocity() {
    Math::Vector2D baseVelocity(
        randomFloat(minVelocity.x, maxVelocity.x),
        randomFloat(minVelocity.y, maxVelocity.y)
    );
    
    if (shapeConfig.type == EmitterShape::Cone) {
        float halfAngle = shapeConfig.angle * 0.5f;
        float randomAngle = randomFloat(-halfAngle, halfAngle);
        
        float dirAngle = std::atan2(shapeConfig.direction.y, shapeConfig.direction.x);
        float finalAngle = dirAngle + randomAngle;
        
        float speed = baseVelocity.magnitude();
        return Math::Vector2D(
            std::cos(finalAngle) * speed,
            std::sin(finalAngle) * speed
        );
    }
    
    return baseVelocity;
}

void ParticleEmitter::updateParticle(Particle& particle, float deltaTime) {
    particle.age += deltaTime;
    
    if (particle.age >= particle.lifetime) {
        particle.active = false;
        --activeParticles;
        return;
    }
    
    float t = particle.age / particle.lifetime;
    
    particle.velocity = particle.velocity + particle.acceleration * deltaTime;
    particle.position = particle.position + particle.velocity * deltaTime;
    particle.rotation += particle.angularVelocity * deltaTime;
    
    particle.scale = startScale + (endScale - startScale) * t;
    
    for (int i = 0; i < 4; ++i) {
        particle.color[i] = startColor[i] + (endColor[i] - startColor[i]) * t;
    }
}

CircleEmitter::CircleEmitter(float radius, bool emitFromEdge)
    : radius(radius), emitFromEdge(emitFromEdge) {}

Math::Vector2D CircleEmitter::getRandomPosition(const Math::Vector2D& center) const {
    float angle = randomFloat(0, 2.0f * M_PI);
    float r = emitFromEdge ? radius : randomFloat(0, radius);
    
    return center + Math::Vector2D(
        std::cos(angle) * r,
        std::sin(angle) * r
    );
}

Math::Vector2D CircleEmitter::getRandomDirection() const {
    float angle = randomFloat(0, 2.0f * M_PI);
    return Math::Vector2D(std::cos(angle), std::sin(angle));
}

RectangleEmitter::RectangleEmitter(float width, float height)
    : width(width), height(height) {}

Math::Vector2D RectangleEmitter::getRandomPosition(const Math::Vector2D& center) const {
    float x = randomFloat(-width * 0.5f, width * 0.5f);
    float y = randomFloat(-height * 0.5f, height * 0.5f);
    return center + Math::Vector2D(x, y);
}

ConeEmitter::ConeEmitter(float angle, const Math::Vector2D& direction)
    : angle(angle), direction(direction) {}

Math::Vector2D ConeEmitter::getRandomDirection() const {
    float halfAngle = angle * 0.5f;
    float randomAngle = randomFloat(-halfAngle, halfAngle);
    
    float dirAngle = std::atan2(direction.y, direction.x);
    float finalAngle = dirAngle + randomAngle;
    
    return Math::Vector2D(std::cos(finalAngle), std::sin(finalAngle));
}

} // namespace Particles
} // namespace JJM
