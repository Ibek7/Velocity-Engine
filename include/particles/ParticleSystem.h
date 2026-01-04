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

/**
 * @brief Sub-emitter trigger conditions
 */
enum class SubEmitterTrigger {
    Birth,          // When parent particle is born
    Death,          // When parent particle dies
    Collision,      // When parent particle collides
    Manual,         // Manually triggered
    Lifetime        // At specific lifetime percentage
};

/**
 * @brief Sub-emitter configuration
 */
struct SubEmitterConfig {
    SubEmitterTrigger trigger;
    float triggerProbability;        // 0-1 chance to trigger
    int particlesPerTrigger;
    float inheritVelocity;           // 0-1 velocity inheritance
    float inheritScale;              // 0-1 scale inheritance
    float inheritRotation;           // 0-1 rotation inheritance
    bool inheritColor;
    float lifetimeThreshold;         // For Lifetime trigger (0-1)
    
    SubEmitterConfig()
        : trigger(SubEmitterTrigger::Death)
        , triggerProbability(1.0f)
        , particlesPerTrigger(5)
        , inheritVelocity(0.5f)
        , inheritScale(0.5f)
        , inheritRotation(0.0f)
        , inheritColor(true)
        , lifetimeThreshold(0.5f)
    {}
};

/**
 * @brief Sub-emitter for spawning particles from other particles
 */
class SubEmitter {
public:
    SubEmitter(ParticleEmitter* emitterTemplate, const SubEmitterConfig& config);
    ~SubEmitter();
    
    void trigger(const Particle& parentParticle);
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);
    
    void setConfig(const SubEmitterConfig& config) { this->config = config; }
    const SubEmitterConfig& getConfig() const { return config; }
    
    ParticleEmitter* getEmitter() { return emitter; }
    bool isActive() const { return active; }
    
private:
    ParticleEmitter* emitter;
    SubEmitterConfig config;
    bool active;
    
    void applyInheritance(Particle& childParticle, const Particle& parentParticle);
};

/**
 * @brief Particle affector types
 */
enum class AffectorType {
    Force,          // Constant force
    Attractor,      // Point attraction
    Repeller,       // Point repulsion
    Vortex,         // Spiral motion
    Turbulence,     // Random turbulence
    Drag,           // Air resistance
    Color,          // Color over lifetime
    Scale,          // Scale over lifetime
    Rotation        // Rotation over lifetime
};

/**
 * @brief Base particle affector
 */
class ParticleAffector {
public:
    ParticleAffector(AffectorType type);
    virtual ~ParticleAffector() = default;
    
    virtual void affect(Particle& particle, float deltaTime) = 0;
    
    AffectorType getType() const { return type; }
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    void setStrength(float strength) { this->strength = strength; }
    float getStrength() const { return strength; }

protected:
    AffectorType type;
    bool enabled;
    float strength;
};

/**
 * @brief Force affector - applies constant force
 */
class ForceAffector : public ParticleAffector {
public:
    ForceAffector(const Math::Vector2D& force);
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setForce(const Math::Vector2D& force) { this->force = force; }
    const Math::Vector2D& getForce() const { return force; }

private:
    Math::Vector2D force;
};

/**
 * @brief Attractor affector - pulls particles toward a point
 */
class AttractorAffector : public ParticleAffector {
public:
    AttractorAffector(const Math::Vector2D& position, float force, float radius);
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    const Math::Vector2D& getPosition() const { return position; }
    void setRadius(float r) { radius = r; }
    float getRadius() const { return radius; }
    void setFalloff(float f) { falloff = f; }

private:
    Math::Vector2D position;
    float radius;
    float falloff;
};

/**
 * @brief Vortex affector - creates spiral motion
 */
class VortexAffector : public ParticleAffector {
public:
    VortexAffector(const Math::Vector2D& center, float rotationSpeed, float pullStrength);
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setCenter(const Math::Vector2D& c) { center = c; }
    void setRotationSpeed(float speed) { rotationSpeed = speed; }
    void setPullStrength(float pull) { pullStrength = pull; }

private:
    Math::Vector2D center;
    float rotationSpeed;
    float pullStrength;
};

/**
 * @brief Turbulence affector - adds random motion
 */
class TurbulenceAffector : public ParticleAffector {
public:
    TurbulenceAffector(float frequency, float amplitude);
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setFrequency(float f) { frequency = f; }
    void setAmplitude(float a) { amplitude = a; }

private:
    float frequency;
    float amplitude;
    float time;
};

/**
 * @brief Color gradient for color over lifetime
 */
struct ColorGradient {
    struct ColorStop {
        float position;  // 0-1
        Graphics::Color color;
    };
    
    std::vector<ColorStop> stops;
    
    void addStop(float position, const Graphics::Color& color);
    Graphics::Color evaluate(float t) const;
};

/**
 * @brief Color over lifetime affector
 */
class ColorOverLifetimeAffector : public ParticleAffector {
public:
    ColorOverLifetimeAffector();
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setGradient(const ColorGradient& gradient) { this->gradient = gradient; }
    ColorGradient& getGradient() { return gradient; }

private:
    ColorGradient gradient;
};

/**
 * @brief Scale over lifetime affector
 */
class ScaleOverLifetimeAffector : public ParticleAffector {
public:
    ScaleOverLifetimeAffector(float startScale, float endScale);
    
    void affect(Particle& particle, float deltaTime) override;
    
    void setStartScale(float scale) { startScale = scale; }
    void setEndScale(float scale) { endScale = scale; }
    void setScaleCurve(std::function<float(float)> curve) { scaleCurve = curve; }

private:
    float startScale;
    float endScale;
    std::function<float(float)> scaleCurve;
};

/**
 * @brief Advanced particle emitter with sub-emitters and affectors
 */
class AdvancedParticleEmitter : public ParticleEmitter {
public:
    AdvancedParticleEmitter(const Math::Vector2D& pos, int maxParticles = 100);
    ~AdvancedParticleEmitter();
    
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer) override;
    
    // Sub-emitter management
    SubEmitter* addSubEmitter(ParticleEmitter* emitterTemplate, const SubEmitterConfig& config);
    void removeSubEmitter(SubEmitter* subEmitter);
    void clearSubEmitters();
    size_t getSubEmitterCount() const { return subEmitters.size(); }
    
    // Affector management
    template<typename T, typename... Args>
    T* addAffector(Args&&... args);
    void removeAffector(ParticleAffector* affector);
    void clearAffectors();
    size_t getAffectorCount() const { return affectors.size(); }
    
    // Event callbacks
    void setOnParticleBirth(std::function<void(Particle&)> callback) { onParticleBirth = callback; }
    void setOnParticleDeath(std::function<void(const Particle&)> callback) { onParticleDeath = callback; }
    void setOnParticleCollision(std::function<void(Particle&)> callback) { onParticleCollision = callback; }

private:
    std::vector<std::unique_ptr<SubEmitter>> subEmitters;
    std::vector<std::unique_ptr<ParticleAffector>> affectors;
    
    std::function<void(Particle&)> onParticleBirth;
    std::function<void(const Particle&)> onParticleDeath;
    std::function<void(Particle&)> onParticleCollision;
    
    void triggerSubEmitters(SubEmitterTrigger trigger, const Particle& particle);
    void applyAffectors(Particle& particle, float deltaTime);
};

template<typename T, typename... Args>
T* AdvancedParticleEmitter::addAffector(Args&&... args) {
    auto affector = std::make_unique<T>(std::forward<Args>(args)...);
    T* ptr = affector.get();
    affectors.push_back(std::move(affector));
    return ptr;
}

} // namespace Particles
} // namespace JJM

#endif // PARTICLE_SYSTEM_H
