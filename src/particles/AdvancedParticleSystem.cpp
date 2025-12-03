#include "../../include/particles/AdvancedParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Particles {

// =============================================================================
// AdvancedParticle Implementation
// =============================================================================

AdvancedParticle::AdvancedParticle()
    : lifetime(0), maxLifetime(1.0f), size(1.0f), startSize(1.0f), endSize(1.0f),
      rotation(0), rotationSpeed(0), mass(1.0f), drag(0.0f),
      textureFrame(0), frameTime(0), frameRate(10.0f), active(false) {}

void AdvancedParticle::reset() {
    position = JJM::Math::Vector2D(0, 0);
    velocity = JJM::Math::Vector2D(0, 0);
    acceleration = JJM::Math::Vector2D(0, 0);
    lifetime = 0;
    active = false;
}

void AdvancedParticle::update(float deltaTime) {
    if (!active) return;
    
    // Update lifetime
    lifetime -= deltaTime;
    if (lifetime <= 0) {
        active = false;
        return;
    }
    
    // Physics
    velocity = velocity + acceleration * deltaTime;
    velocity = velocity * (1.0f - drag * deltaTime);
    position = position + velocity * deltaTime;
    
    // Rotation
    rotation += rotationSpeed * deltaTime;
    
    // Interpolate size
    float t = getLifetimePercent();
    size = startSize + (endSize - startSize) * t;
    
    // Interpolate color
    color.r = static_cast<uint8_t>(startColor.r + (endColor.r - startColor.r) * t);
    color.g = static_cast<uint8_t>(startColor.g + (endColor.g - startColor.g) * t);
    color.b = static_cast<uint8_t>(startColor.b + (endColor.b - startColor.b) * t);
    color.a = static_cast<uint8_t>(startColor.a + (endColor.a - startColor.a) * t);
    
    // Texture animation
    frameTime += deltaTime;
    if (frameTime >= 1.0f / frameRate) {
        textureFrame++;
        frameTime = 0;
    }
}

float AdvancedParticle::getLifetimePercent() const {
    return 1.0f - (lifetime / maxLifetime);
}

// =============================================================================
// ColorOverLifetimeModifier Implementation
// =============================================================================

ColorOverLifetimeModifier::ColorOverLifetimeModifier() {}

void ColorOverLifetimeModifier::addColorKey(float time, const JJM::Graphics::Color& color) {
    colorKeys.emplace_back(time, color);
    std::sort(colorKeys.begin(), colorKeys.end(),
             [](const ColorKey& a, const ColorKey& b) { return a.time < b.time; });
}

void ColorOverLifetimeModifier::apply(AdvancedParticle& particle, float) {
    if (!enabled || colorKeys.empty()) return;
    
    particle.color = interpolateColor(particle.getLifetimePercent());
}

std::unique_ptr<ParticleModifier> ColorOverLifetimeModifier::clone() const {
    auto clone = std::make_unique<ColorOverLifetimeModifier>();
    clone->colorKeys = colorKeys;
    clone->enabled = enabled;
    return clone;
}

JJM::Graphics::Color ColorOverLifetimeModifier::interpolateColor(float t) const {
    if (colorKeys.empty()) return JJM::Graphics::Color();
    if (colorKeys.size() == 1) return colorKeys[0].color;
    
    if (t <= colorKeys.front().time) return colorKeys.front().color;
    if (t >= colorKeys.back().time) return colorKeys.back().color;
    
    for (size_t i = 0; i < colorKeys.size() - 1; i++) {
        if (t >= colorKeys[i].time && t <= colorKeys[i + 1].time) {
            float localT = (t - colorKeys[i].time) / (colorKeys[i + 1].time - colorKeys[i].time);
            
            const auto& c1 = colorKeys[i].color;
            const auto& c2 = colorKeys[i + 1].color;
            
            return JJM::Graphics::Color(
                static_cast<uint8_t>(c1.r + (c2.r - c1.r) * localT),
                static_cast<uint8_t>(c1.g + (c2.g - c1.g) * localT),
                static_cast<uint8_t>(c1.b + (c2.b - c1.b) * localT),
                static_cast<uint8_t>(c1.a + (c2.a - c1.a) * localT)
            );
        }
    }
    
    return colorKeys.back().color;
}

// =============================================================================
// SizeOverLifetimeModifier Implementation
// =============================================================================

SizeOverLifetimeModifier::SizeOverLifetimeModifier() {}

void SizeOverLifetimeModifier::addSizeKey(float time, float size) {
    sizeKeys.emplace_back(time, size);
    std::sort(sizeKeys.begin(), sizeKeys.end(),
             [](const SizeKey& a, const SizeKey& b) { return a.time < b.time; });
}

void SizeOverLifetimeModifier::apply(AdvancedParticle& particle, float) {
    if (!enabled || sizeKeys.empty()) return;
    
    particle.size = interpolateSize(particle.getLifetimePercent());
}

std::unique_ptr<ParticleModifier> SizeOverLifetimeModifier::clone() const {
    auto clone = std::make_unique<SizeOverLifetimeModifier>();
    clone->sizeKeys = sizeKeys;
    clone->enabled = enabled;
    return clone;
}

float SizeOverLifetimeModifier::interpolateSize(float t) const {
    if (sizeKeys.empty()) return 1.0f;
    if (sizeKeys.size() == 1) return sizeKeys[0].size;
    
    if (t <= sizeKeys.front().time) return sizeKeys.front().size;
    if (t >= sizeKeys.back().time) return sizeKeys.back().size;
    
    for (size_t i = 0; i < sizeKeys.size() - 1; i++) {
        if (t >= sizeKeys[i].time && t <= sizeKeys[i + 1].time) {
            float localT = (t - sizeKeys[i].time) / (sizeKeys[i + 1].time - sizeKeys[i].time);
            return sizeKeys[i].size + (sizeKeys[i + 1].size - sizeKeys[i].size) * localT;
        }
    }
    
    return sizeKeys.back().size;
}

// =============================================================================
// Other Modifiers Implementation
// =============================================================================

VelocityOverLifetimeModifier::VelocityOverLifetimeModifier(const JJM::Math::Vector2D& velocity)
    : velocity(velocity) {}

void VelocityOverLifetimeModifier::apply(AdvancedParticle& particle, float deltaTime) {
    if (!enabled) return;
    
    if (additive) {
        particle.velocity = particle.velocity + velocity * deltaTime;
    } else {
        particle.velocity = velocity;
    }
}

std::unique_ptr<ParticleModifier> VelocityOverLifetimeModifier::clone() const {
    auto clone = std::make_unique<VelocityOverLifetimeModifier>(velocity);
    clone->additive = additive;
    clone->enabled = enabled;
    return clone;
}

OrbitalVelocityModifier::OrbitalVelocityModifier(const JJM::Math::Vector2D& center, float speed)
    : center(center), orbitalSpeed(speed), radialSpeed(0.0f) {}

void OrbitalVelocityModifier::apply(AdvancedParticle& particle, float deltaTime) {
    if (!enabled) return;
    
    JJM::Math::Vector2D toCenter = center - particle.position;
    float distance = toCenter.magnitude();
    
    if (distance < 0.001f) return;
    
    toCenter.normalize();
    
    // Tangent direction for orbital motion
    JJM::Math::Vector2D tangent(-toCenter.y, toCenter.x);
    
    // Apply orbital and radial velocities
    particle.velocity = particle.velocity + tangent * orbitalSpeed * deltaTime;
    particle.velocity = particle.velocity + toCenter * radialSpeed * deltaTime;
}

std::unique_ptr<ParticleModifier> OrbitalVelocityModifier::clone() const {
    auto clone = std::make_unique<OrbitalVelocityModifier>(center, orbitalSpeed);
    clone->radialSpeed = radialSpeed;
    clone->enabled = enabled;
    return clone;
}

TurbulenceModifier::TurbulenceModifier(float strength, float frequency)
    : strength(strength), frequency(frequency), time(0.0f) {}

void TurbulenceModifier::apply(AdvancedParticle& particle, float deltaTime) {
    if (!enabled) return;
    
    time += deltaTime;
    
    float nx = noise(particle.position.x * frequency, time);
    float ny = noise(particle.position.y * frequency, time + 100.0f);
    
    particle.velocity = particle.velocity + JJM::Math::Vector2D(nx, ny) * strength * deltaTime;
}

std::unique_ptr<ParticleModifier> TurbulenceModifier::clone() const {
    auto clone = std::make_unique<TurbulenceModifier>(strength, frequency);
    clone->time = time;
    clone->enabled = enabled;
    return clone;
}

float TurbulenceModifier::noise(float x, float y) const {
    // Simple pseudo-random noise
    return std::sin(x * 12.9898f + y * 78.233f) * 2.0f - 1.0f;
}

DragModifier::DragModifier(float coefficient) : dragCoefficient(coefficient) {}

void DragModifier::apply(AdvancedParticle& particle, float deltaTime) {
    if (!enabled) return;
    
    particle.velocity = particle.velocity * (1.0f - dragCoefficient * deltaTime);
}

std::unique_ptr<ParticleModifier> DragModifier::clone() const {
    auto clone = std::make_unique<DragModifier>(dragCoefficient);
    clone->enabled = enabled;
    return clone;
}

// =============================================================================
// Force Fields Implementation
// =============================================================================

PointAttractorField::PointAttractorField(const JJM::Math::Vector2D& position, float strength)
    : position(position), radius(100.0f), repel(false) {
    this->strength = strength;
}

JJM::Math::Vector2D PointAttractorField::calculateForce(const AdvancedParticle& particle) const {
    if (!enabled) return JJM::Math::Vector2D(0, 0);
    
    JJM::Math::Vector2D dir = position - particle.position;
    float dist = dir.magnitude();
    
    if (dist > radius || dist < 0.001f) return JJM::Math::Vector2D(0, 0);
    
    dir.normalize();
    
    float force = strength / (dist * dist + 1.0f);
    
    if (repel) {
        dir = dir * -1.0f;
    }
    
    return dir * force;
}

std::unique_ptr<ParticleForceField> PointAttractorField::clone() const {
    auto clone = std::make_unique<PointAttractorField>(position, strength);
    clone->radius = radius;
    clone->repel = repel;
    clone->enabled = enabled;
    return clone;
}

DirectionalForceField::DirectionalForceField(const JJM::Math::Vector2D& direction, float strength)
    : direction(direction) {
    this->strength = strength;
}

JJM::Math::Vector2D DirectionalForceField::calculateForce(const AdvancedParticle&) const {
    if (!enabled) return JJM::Math::Vector2D(0, 0);
    
    return direction * strength;
}

std::unique_ptr<ParticleForceField> DirectionalForceField::clone() const {
    auto clone = std::make_unique<DirectionalForceField>(direction, strength);
    clone->enabled = enabled;
    return clone;
}

VortexForceField::VortexForceField(const JJM::Math::Vector2D& center, float strength)
    : center(center), radius(100.0f), inwardStrength(0.0f) {
    this->strength = strength;
}

JJM::Math::Vector2D VortexForceField::calculateForce(const AdvancedParticle& particle) const {
    if (!enabled) return JJM::Math::Vector2D(0, 0);
    
    JJM::Math::Vector2D toCenter = center - particle.position;
    float dist = toCenter.magnitude();
    
    if (dist > radius || dist < 0.001f) return JJM::Math::Vector2D(0, 0);
    
    toCenter.normalize();
    
    // Tangent for vortex
    JJM::Math::Vector2D tangent(-toCenter.y, toCenter.x);
    
    JJM::Math::Vector2D vortexForce = tangent * strength;
    JJM::Math::Vector2D inwardForce = toCenter * inwardStrength;
    
    return vortexForce + inwardForce;
}

std::unique_ptr<ParticleForceField> VortexForceField::clone() const {
    auto clone = std::make_unique<VortexForceField>(center, strength);
    clone->radius = radius;
    clone->inwardStrength = inwardStrength;
    clone->enabled = enabled;
    return clone;
}

// =============================================================================
// ParticlePool Implementation
// =============================================================================

ParticlePool::ParticlePool(size_t initialSize) : activeParticles(0) {
    particles.reserve(initialSize);
    for (size_t i = 0; i < initialSize; i++) {
        particles.push_back(std::make_unique<AdvancedParticle>());
        available.push_back(particles.back().get());
    }
}

ParticlePool::~ParticlePool() {
    clear();
}

AdvancedParticle* ParticlePool::acquire() {
    if (available.empty()) {
        // Grow pool
        particles.push_back(std::make_unique<AdvancedParticle>());
        available.push_back(particles.back().get());
    }
    
    AdvancedParticle* particle = available.back();
    available.pop_back();
    activeParticles++;
    return particle;
}

void ParticlePool::release(AdvancedParticle* particle) {
    if (particle) {
        particle->reset();
        available.push_back(particle);
        if (activeParticles > 0) activeParticles--;
    }
}

void ParticlePool::clear() {
    available.clear();
    for (auto& particle : particles) {
        particle->reset();
        available.push_back(particle.get());
    }
    activeParticles = 0;
}

// =============================================================================
// AdvancedParticleEmitter Implementation (continued in next section due to size)
// =============================================================================

AdvancedParticleEmitter::AdvancedParticleEmitter(ParticlePool& pool, size_t maxParticles)
    : pool(pool), emissionShape(EmissionShape::POINT), shapeRadius(1.0f),
      emissionAngle(0.0f), emissionArc(360.0f), maxParticles(maxParticles),
      emissionRate(10.0f), emissionTimer(0.0f), active(false), paused(false),
      looping(true), duration(5.0f), playTime(0.0f),
      minLifetime(1.0f), maxLifetime(3.0f), minSpeed(10.0f), maxSpeed(50.0f),
      minSize(1.0f), maxSize(5.0f), minRotation(0.0f), maxRotation(360.0f),
      minRotationSpeed(-90.0f), maxRotationSpeed(90.0f),
      minMass(1.0f), maxMass(1.0f), particleDrag(0.0f),
      colorVariation(0.1f), textureFrames(1), frameRate(10.0f),
      rng(std::random_device{}()), dist(0.0f, 1.0f) {
    
    activeParticles.reserve(maxParticles);
}

AdvancedParticleEmitter::~AdvancedParticleEmitter() {
    reset();
}

void AdvancedParticleEmitter::update(float deltaTime) {
    if (!active || paused) return;
    
    playTime += deltaTime;
    
    // Check duration
    if (!looping && playTime >= duration) {
        active = false;
    }
    
    // Emission
    if (active) {
        emissionTimer += deltaTime;
        float emissionInterval = 1.0f / emissionRate;
        
        while (emissionTimer >= emissionInterval && activeParticles.size() < maxParticles) {
            createParticle();
            emissionTimer -= emissionInterval;
        }
    }
    
    // Update particles
    for (auto it = activeParticles.begin(); it != activeParticles.end();) {
        AdvancedParticle* particle = *it;
        
        // Apply force fields
        for (const auto& field : forceFields) {
            if (field && field->enabled) {
                JJM::Math::Vector2D force = field->calculateForce(*particle);
                particle->acceleration = particle->acceleration + force / particle->mass;
            }
        }
        
        // Apply modifiers
        for (const auto& modifier : modifiers) {
            if (modifier && modifier->enabled) {
                modifier->apply(*particle, deltaTime);
            }
        }
        
        // Update particle
        particle->update(deltaTime);
        
        // Remove if dead
        if (!particle->isAlive()) {
            pool.release(particle);
            it = activeParticles.erase(it);
        } else {
            ++it;
        }
    }
}

void AdvancedParticleEmitter::render(JJM::Graphics::Renderer* renderer) {
    if (!renderer) return;
    
    for (const auto* particle : activeParticles) {
        if (particle && particle->isAlive()) {
            renderer->drawRect(particle->position, 
                             JJM::Math::Vector2D(particle->size, particle->size),
                             particle->color, true);
        }
    }
}

void AdvancedParticleEmitter::emit(size_t count) {
    for (size_t i = 0; i < count && activeParticles.size() < maxParticles; i++) {
        createParticle();
    }
}

void AdvancedParticleEmitter::emitBurst(size_t count) {
    emit(count);
}

void AdvancedParticleEmitter::start() {
    active = true;
    paused = false;
    playTime = 0.0f;
}

void AdvancedParticleEmitter::stop() {
    active = false;
    reset();
}

void AdvancedParticleEmitter::pause() {
    paused = true;
}

void AdvancedParticleEmitter::resume() {
    paused = false;
}

void AdvancedParticleEmitter::reset() {
    for (auto* particle : activeParticles) {
        pool.release(particle);
    }
    activeParticles.clear();
    emissionTimer = 0.0f;
    playTime = 0.0f;
}

void AdvancedParticleEmitter::setLifetime(float min, float max) {
    minLifetime = min;
    maxLifetime = max;
}

void AdvancedParticleEmitter::setSpeed(float min, float max) {
    minSpeed = min;
    maxSpeed = max;
}

void AdvancedParticleEmitter::setSize(float min, float max) {
    minSize = min;
    maxSize = max;
}

void AdvancedParticleEmitter::setRotation(float min, float max) {
    minRotation = min;
    maxRotation = max;
}

void AdvancedParticleEmitter::setRotationSpeed(float min, float max) {
    minRotationSpeed = min;
    maxRotationSpeed = max;
}

void AdvancedParticleEmitter::setMass(float min, float max) {
    minMass = min;
    maxMass = max;
}

void AdvancedParticleEmitter::addModifier(std::unique_ptr<ParticleModifier> modifier) {
    if (modifier) {
        modifiers.push_back(std::move(modifier));
    }
}

void AdvancedParticleEmitter::addForceField(std::unique_ptr<ParticleForceField> field) {
    if (field) {
        forceFields.push_back(std::move(field));
    }
}

void AdvancedParticleEmitter::clearModifiers() {
    modifiers.clear();
}

void AdvancedParticleEmitter::clearForceFields() {
    forceFields.clear();
}

void AdvancedParticleEmitter::setPrewarm(bool prewarm) {
    if (prewarm) {
        // Simulate to fill with particles
        for (int i = 0; i < 10; i++) {
            update(duration / 10.0f);
        }
    }
}

void AdvancedParticleEmitter::createParticle() {
    AdvancedParticle* particle = pool.acquire();
    if (!particle) return;
    
    particle->active = true;
    particle->position = getEmissionPoint();
    
    JJM::Math::Vector2D direction = getEmissionDirection();
    float speed = randomFloat(minSpeed, maxSpeed);
    particle->velocity = direction * speed;
    
    particle->lifetime = randomFloat(minLifetime, maxLifetime);
    particle->maxLifetime = particle->lifetime;
    
    particle->startSize = randomFloat(minSize, maxSize);
    particle->endSize = particle->startSize * 0.2f;
    particle->size = particle->startSize;
    
    particle->rotation = randomFloat(minRotation, maxRotation);
    particle->rotationSpeed = randomFloat(minRotationSpeed, maxRotationSpeed);
    
    particle->mass = randomFloat(minMass, maxMass);
    particle->drag = particleDrag;
    
    particle->startColor = varyColor(startColor, colorVariation);
    particle->endColor = varyColor(endColor, colorVariation);
    particle->color = particle->startColor;
    
    particle->textureFrame = 0;
    particle->frameTime = 0.0f;
    particle->frameRate = frameRate;
    
    activeParticles.push_back(particle);
}

JJM::Math::Vector2D AdvancedParticleEmitter::getEmissionPoint() const {
    switch (emissionShape) {
        case EmissionShape::CIRCLE: {
            float angle = randomFloat(0.0f, 360.0f) * 3.14159f / 180.0f;
            float r = randomFloat(0.0f, shapeRadius);
            return position + JJM::Math::Vector2D(std::cos(angle) * r, std::sin(angle) * r);
        }
        case EmissionShape::RING: {
            float angle = randomFloat(0.0f, 360.0f) * 3.14159f / 180.0f;
            return position + JJM::Math::Vector2D(std::cos(angle), std::sin(angle)) * shapeRadius;
        }
        case EmissionShape::RECTANGLE: {
            float x = randomFloat(-shapeSize.x / 2, shapeSize.x / 2);
            float y = randomFloat(-shapeSize.y / 2, shapeSize.y / 2);
            return position + JJM::Math::Vector2D(x, y);
        }
        case EmissionShape::LINE: {
            float t = randomFloat(0.0f, 1.0f);
            return position + JJM::Math::Vector2D(shapeSize.x * (t - 0.5f), 0);
        }
        default:
            return position;
    }
}

JJM::Math::Vector2D AdvancedParticleEmitter::getEmissionDirection() const {
    float angle = emissionAngle + randomFloat(-emissionArc / 2, emissionArc / 2);
    angle = angle * 3.14159f / 180.0f;
    
    return JJM::Math::Vector2D(std::cos(angle), std::sin(angle));
}

float AdvancedParticleEmitter::randomFloat(float min, float max) const {
    auto& mutableRng = const_cast<std::mt19937&>(rng);
    auto& mutableDist = const_cast<std::uniform_real_distribution<float>&>(dist);
    return min + mutableDist(mutableRng) * (max - min);
}

JJM::Graphics::Color AdvancedParticleEmitter::varyColor(const JJM::Graphics::Color& color, float variation) {
    auto vary = [this, variation](uint8_t value) {
        float v = value + randomFloat(-variation * 255, variation * 255);
        return static_cast<uint8_t>(std::max(0.0f, std::min(255.0f, v)));
    };
    
    return JJM::Graphics::Color(vary(color.r), vary(color.g), vary(color.b), color.a);
}

// =============================================================================
// AdvancedParticleSystem Implementation
// =============================================================================

AdvancedParticleSystem::AdvancedParticleSystem(size_t poolSize)
    : pool(std::make_unique<ParticlePool>(poolSize)), simulationSpeed(1.0f) {}

AdvancedParticleSystem::~AdvancedParticleSystem() {
    removeAllEmitters();
}

AdvancedParticleEmitter* AdvancedParticleSystem::createEmitter(const JJM::Math::Vector2D& position,
                                                               size_t maxParticles) {
    auto emitter = std::make_unique<AdvancedParticleEmitter>(*pool, maxParticles);
    emitter->setPosition(position);
    
    AdvancedParticleEmitter* ptr = emitter.get();
    emitters.push_back(std::move(emitter));
    
    return ptr;
}

void AdvancedParticleSystem::removeEmitter(AdvancedParticleEmitter* emitter) {
    emitters.erase(std::remove_if(emitters.begin(), emitters.end(),
                                 [emitter](const std::unique_ptr<AdvancedParticleEmitter>& e) {
                                     return e.get() == emitter;
                                 }),
                  emitters.end());
}

void AdvancedParticleSystem::removeAllEmitters() {
    emitters.clear();
}

void AdvancedParticleSystem::update(float deltaTime) {
    float scaledDelta = deltaTime * simulationSpeed;
    
    for (auto& emitter : emitters) {
        emitter->update(scaledDelta);
    }
}

void AdvancedParticleSystem::render(JJM::Graphics::Renderer* renderer) {
    for (auto& emitter : emitters) {
        emitter->render(renderer);
    }
}

void AdvancedParticleSystem::addGlobalModifier(std::unique_ptr<ParticleModifier> modifier) {
    if (modifier) {
        globalModifiers.push_back(std::move(modifier));
    }
}

void AdvancedParticleSystem::addGlobalForceField(std::unique_ptr<ParticleForceField> field) {
    if (field) {
        globalForceFields.push_back(std::move(field));
    }
}

void AdvancedParticleSystem::clearGlobalModifiers() {
    globalModifiers.clear();
}

void AdvancedParticleSystem::clearGlobalForceFields() {
    globalForceFields.clear();
}

size_t AdvancedParticleSystem::getTotalParticleCount() const {
    size_t total = 0;
    for (const auto& emitter : emitters) {
        total += emitter->getActiveParticleCount();
    }
    return total;
}

void AdvancedParticleSystem::setMaxParticles(size_t max) {
    // Resize pool - simplified
}

// =============================================================================
// Particle Effect Presets
// =============================================================================

namespace ParticleEffects {

AdvancedParticleEmitter* createFireEffect(AdvancedParticleSystem& system,
                                         const JJM::Math::Vector2D& position) {
    auto* emitter = system.createEmitter(position, 200);
    emitter->setEmissionShape(EmissionShape::CIRCLE);
    emitter->setShapeRadius(5.0f);
    emitter->setEmissionRate(50.0f);
    emitter->setLifetime(0.5f, 1.5f);
    emitter->setSpeed(20.0f, 50.0f);
    emitter->setSize(2.0f, 8.0f);
    emitter->setStartColor(JJM::Graphics::Color(255, 200, 0, 255));
    emitter->setEndColor(JJM::Graphics::Color(255, 50, 0, 0));
    
    // Add upward force
    emitter->addForceField(std::make_unique<DirectionalForceField>(
        JJM::Math::Vector2D(0, -50), 1.0f));
    
    emitter->start();
    return emitter;
}

AdvancedParticleEmitter* createSmokeEffect(AdvancedParticleSystem& system,
                                          const JJM::Math::Vector2D& position) {
    auto* emitter = system.createEmitter(position, 100);
    emitter->setEmissionShape(EmissionShape::POINT);
    emitter->setEmissionRate(20.0f);
    emitter->setLifetime(2.0f, 4.0f);
    emitter->setSpeed(5.0f, 15.0f);
    emitter->setSize(5.0f, 15.0f);
    emitter->setStartColor(JJM::Graphics::Color(100, 100, 100, 200));
    emitter->setEndColor(JJM::Graphics::Color(150, 150, 150, 0));
    
    emitter->addModifier(std::make_unique<TurbulenceModifier>(10.0f, 0.1f));
    emitter->addForceField(std::make_unique<DirectionalForceField>(
        JJM::Math::Vector2D(0, -20), 1.0f));
    
    emitter->start();
    return emitter;
}

AdvancedParticleEmitter* createExplosionEffect(AdvancedParticleSystem& system,
                                               const JJM::Math::Vector2D& position) {
    auto* emitter = system.createEmitter(position, 500);
    emitter->setEmissionShape(EmissionShape::POINT);
    emitter->setLifetime(0.3f, 1.0f);
    emitter->setSpeed(100.0f, 300.0f);
    emitter->setSize(2.0f, 6.0f);
    emitter->setStartColor(JJM::Graphics::Color(255, 255, 200, 255));
    emitter->setEndColor(JJM::Graphics::Color(255, 100, 0, 0));
    emitter->setLooping(false);
    
    emitter->emitBurst(500);
    return emitter;
}

AdvancedParticleEmitter* createSparkEffect(AdvancedParticleSystem& system,
                                          const JJM::Math::Vector2D& position) {
    auto* emitter = system.createEmitter(position, 100);
    emitter->setEmissionShape(EmissionShape::POINT);
    emitter->setEmissionRate(100.0f);
    emitter->setLifetime(0.2f, 0.8f);
    emitter->setSpeed(50.0f, 150.0f);
    emitter->setSize(1.0f, 3.0f);
    emitter->setStartColor(JJM::Graphics::Color(255, 255, 100, 255));
    emitter->setEndColor(JJM::Graphics::Color(255, 200, 0, 0));
    
    emitter->addForceField(std::make_unique<DirectionalForceField>(
        JJM::Math::Vector2D(0, 100), 1.0f));
    
    emitter->start();
    return emitter;
}

AdvancedParticleEmitter* createMagicEffect(AdvancedParticleSystem& system,
                                          const JJM::Math::Vector2D& position) {
    auto* emitter = system.createEmitter(position, 300);
    emitter->setEmissionShape(EmissionShape::CIRCLE);
    emitter->setShapeRadius(20.0f);
    emitter->setEmissionRate(50.0f);
    emitter->setLifetime(1.0f, 2.0f);
    emitter->setSpeed(10.0f, 30.0f);
    emitter->setSize(2.0f, 5.0f);
    emitter->setStartColor(JJM::Graphics::Color(150, 50, 255, 255));
    emitter->setEndColor(JJM::Graphics::Color(200, 100, 255, 0));
    
    emitter->addModifier(std::make_unique<OrbitalVelocityModifier>(position, 100.0f));
    
    emitter->start();
    return emitter;
}

AdvancedParticleEmitter* createRainEffect(AdvancedParticleSystem& system,
                                         const JJM::Math::Vector2D& position,
                                         const JJM::Math::Vector2D& size) {
    auto* emitter = system.createEmitter(position, 1000);
    emitter->setEmissionShape(EmissionShape::RECTANGLE);
    emitter->setShapeSize(size);
    emitter->setEmissionRate(200.0f);
    emitter->setLifetime(2.0f, 4.0f);
    emitter->setSpeed(200.0f, 300.0f);
    emitter->setSize(1.0f, 2.0f);
    emitter->setEmissionAngle(90.0f);
    emitter->setEmissionArc(10.0f);
    emitter->setStartColor(JJM::Graphics::Color(100, 150, 255, 200));
    emitter->setEndColor(JJM::Graphics::Color(100, 150, 255, 100));
    
    emitter->start();
    return emitter;
}

} // namespace ParticleEffects

} // namespace Particles
} // namespace JJM
