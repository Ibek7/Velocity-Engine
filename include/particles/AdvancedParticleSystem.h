#ifndef ADVANCED_PARTICLE_SYSTEM_H
#define ADVANCED_PARTICLE_SYSTEM_H

#include "../math/Vector2D.h"
#include "../graphics/Color.h"
#include "../graphics/Renderer.h"
#include <vector>
#include <memory>
#include <functional>
#include <random>

namespace JJM {
namespace Particles {

// Forward declarations
struct AdvancedParticle;
class ParticleModifier;
class ParticleForceField;
class AdvancedParticleEmitter;
class ParticlePool;

// Advanced particle with more properties
struct AdvancedParticle {
    JJM::Math::Vector2D position;
    JJM::Math::Vector2D velocity;
    JJM::Math::Vector2D acceleration;
    
    JJM::Graphics::Color color;
    JJM::Graphics::Color startColor;
    JJM::Graphics::Color endColor;
    
    float lifetime;
    float maxLifetime;
    float size;
    float startSize;
    float endSize;
    float rotation;
    float rotationSpeed;
    float mass;
    float drag;
    
    // Texture animation
    int textureFrame;
    float frameTime;
    float frameRate;
    
    // Custom data
    std::vector<float> customData;
    
    bool active;
    
    AdvancedParticle();
    
    void reset();
    void update(float deltaTime);
    bool isAlive() const { return active && lifetime > 0; }
    float getLifetimePercent() const;
};

// Particle modifier base class
class ParticleModifier {
public:
    virtual ~ParticleModifier() = default;
    virtual void apply(AdvancedParticle& particle, float deltaTime) = 0;
    virtual std::unique_ptr<ParticleModifier> clone() const = 0;
    
    bool enabled = true;
};

// Color over lifetime modifier
class ColorOverLifetimeModifier : public ParticleModifier {
public:
    struct ColorKey {
        float time;  // 0-1
        JJM::Graphics::Color color;
        
        ColorKey(float t, const JJM::Graphics::Color& c) : time(t), color(c) {}
    };
    
    ColorOverLifetimeModifier();
    
    void addColorKey(float time, const JJM::Graphics::Color& color);
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
private:
    std::vector<ColorKey> colorKeys;
    
    JJM::Graphics::Color interpolateColor(float t) const;
};

// Size over lifetime modifier
class SizeOverLifetimeModifier : public ParticleModifier {
public:
    struct SizeKey {
        float time;  // 0-1
        float size;
        
        SizeKey(float t, float s) : time(t), size(s) {}
    };
    
    SizeOverLifetimeModifier();
    
    void addSizeKey(float time, float size);
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
private:
    std::vector<SizeKey> sizeKeys;
    
    float interpolateSize(float t) const;
};

// Velocity over lifetime modifier
class VelocityOverLifetimeModifier : public ParticleModifier {
public:
    VelocityOverLifetimeModifier(const JJM::Math::Vector2D& velocity);
    
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
    JJM::Math::Vector2D velocity;
    bool additive = true;
};

// Orbital velocity modifier
class OrbitalVelocityModifier : public ParticleModifier {
public:
    OrbitalVelocityModifier(const JJM::Math::Vector2D& center, float speed);
    
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
    JJM::Math::Vector2D center;
    float orbitalSpeed;
    float radialSpeed;
};

// Turbulence modifier
class TurbulenceModifier : public ParticleModifier {
public:
    TurbulenceModifier(float strength, float frequency);
    
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
    float strength;
    float frequency;
    float time;
    
private:
    float noise(float x, float y) const;
};

// Drag modifier
class DragModifier : public ParticleModifier {
public:
    DragModifier(float coefficient);
    
    void apply(AdvancedParticle& particle, float deltaTime) override;
    std::unique_ptr<ParticleModifier> clone() const override;
    
    float dragCoefficient;
};

// Force field base class
class ParticleForceField {
public:
    virtual ~ParticleForceField() = default;
    virtual JJM::Math::Vector2D calculateForce(const AdvancedParticle& particle) const = 0;
    virtual std::unique_ptr<ParticleForceField> clone() const = 0;
    
    bool enabled = true;
    float strength = 1.0f;
};

// Point attractor force field
class PointAttractorField : public ParticleForceField {
public:
    PointAttractorField(const JJM::Math::Vector2D& position, float strength);
    
    JJM::Math::Vector2D calculateForce(const AdvancedParticle& particle) const override;
    std::unique_ptr<ParticleForceField> clone() const override;
    
    JJM::Math::Vector2D position;
    float radius;
    bool repel;
};

// Directional force field (wind, gravity)
class DirectionalForceField : public ParticleForceField {
public:
    DirectionalForceField(const JJM::Math::Vector2D& direction, float strength);
    
    JJM::Math::Vector2D calculateForce(const AdvancedParticle& particle) const override;
    std::unique_ptr<ParticleForceField> clone() const override;
    
    JJM::Math::Vector2D direction;
};

// Vortex force field
class VortexForceField : public ParticleForceField {
public:
    VortexForceField(const JJM::Math::Vector2D& center, float strength);
    
    JJM::Math::Vector2D calculateForce(const AdvancedParticle& particle) const override;
    std::unique_ptr<ParticleForceField> clone() const override;
    
    JJM::Math::Vector2D center;
    float radius;
    float inwardStrength;
};

// Particle pool for efficient memory management
class ParticlePool {
public:
    ParticlePool(size_t initialSize = 1000);
    ~ParticlePool();
    
    AdvancedParticle* acquire();
    void release(AdvancedParticle* particle);
    
    void clear();
    size_t size() const { return particles.size(); }
    size_t activeCount() const { return activeParticles; }
    
private:
    std::vector<std::unique_ptr<AdvancedParticle>> particles;
    std::vector<AdvancedParticle*> available;
    size_t activeParticles;
};

// Emission shape
enum class EmissionShape {
    POINT,
    CIRCLE,
    RING,
    RECTANGLE,
    LINE,
    CONE
};

// Advanced particle emitter
class AdvancedParticleEmitter {
public:
    AdvancedParticleEmitter(ParticlePool& pool, size_t maxParticles = 1000);
    ~AdvancedParticleEmitter();
    
    // Update and rendering
    void update(float deltaTime);
    void render(JJM::Graphics::Renderer* renderer);
    
    // Emission control
    void emit(size_t count = 1);
    void emitBurst(size_t count);
    void start();
    void stop();
    void pause();
    void resume();
    void reset();
    
    // Position and shape
    void setPosition(const JJM::Math::Vector2D& pos) { position = pos; }
    void setEmissionShape(EmissionShape shape) { emissionShape = shape; }
    void setShapeRadius(float radius) { shapeRadius = radius; }
    void setShapeSize(const JJM::Math::Vector2D& size) { shapeSize = size; }
    void setEmissionAngle(float angle) { emissionAngle = angle; }
    void setEmissionArc(float arc) { emissionArc = arc; }
    
    // Emission properties
    void setEmissionRate(float rate) { emissionRate = rate; }
    void setLifetime(float min, float max);
    void setSpeed(float min, float max);
    void setSize(float min, float max);
    void setRotation(float min, float max);
    void setRotationSpeed(float min, float max);
    void setMass(float min, float max);
    void setDrag(float drag) { particleDrag = drag; }
    
    // Color
    void setStartColor(const JJM::Graphics::Color& color) { startColor = color; }
    void setEndColor(const JJM::Graphics::Color& color) { endColor = color; }
    void setColorVariation(float variation) { colorVariation = variation; }
    
    // Modifiers and force fields
    void addModifier(std::unique_ptr<ParticleModifier> modifier);
    void addForceField(std::unique_ptr<ParticleForceField> field);
    void clearModifiers();
    void clearForceFields();
    
    // Texture animation
    void setTextureFrames(int frames) { textureFrames = frames; }
    void setFrameRate(float rate) { frameRate = rate; }
    
    // Configuration
    void setLooping(bool loop) { looping = loop; }
    void setDuration(float dur) { duration = dur; }
    void setPrewarm(bool prewarm);
    
    // Query
    bool isActive() const { return active && !paused; }
    bool isPaused() const { return paused; }
    size_t getActiveParticleCount() const { return activeParticles.size(); }
    const JJM::Math::Vector2D& getPosition() const { return position; }
    
private:
    ParticlePool& pool;
    std::vector<AdvancedParticle*> activeParticles;
    
    JJM::Math::Vector2D position;
    EmissionShape emissionShape;
    float shapeRadius;
    JJM::Math::Vector2D shapeSize;
    float emissionAngle;
    float emissionArc;
    
    size_t maxParticles;
    float emissionRate;
    float emissionTimer;
    bool active;
    bool paused;
    bool looping;
    float duration;
    float playTime;
    
    // Particle properties
    float minLifetime, maxLifetime;
    float minSpeed, maxSpeed;
    float minSize, maxSize;
    float minRotation, maxRotation;
    float minRotationSpeed, maxRotationSpeed;
    float minMass, maxMass;
    float particleDrag;
    
    JJM::Graphics::Color startColor;
    JJM::Graphics::Color endColor;
    float colorVariation;
    
    int textureFrames;
    float frameRate;
    
    std::vector<std::unique_ptr<ParticleModifier>> modifiers;
    std::vector<std::unique_ptr<ParticleForceField>> forceFields;
    
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;
    
    void createParticle();
    JJM::Math::Vector2D getEmissionPoint() const;
    JJM::Math::Vector2D getEmissionDirection() const;
    float randomFloat(float min, float max) const;
    JJM::Graphics::Color varyColor(const JJM::Graphics::Color& color, float variation);
};

// Advanced particle system manager
class AdvancedParticleSystem {
public:
    AdvancedParticleSystem(size_t poolSize = 10000);
    ~AdvancedParticleSystem();
    
    // Emitter management
    AdvancedParticleEmitter* createEmitter(const JJM::Math::Vector2D& position,
                                          size_t maxParticles = 1000);
    void removeEmitter(AdvancedParticleEmitter* emitter);
    void removeAllEmitters();
    
    // Update and rendering
    void update(float deltaTime);
    void render(JJM::Graphics::Renderer* renderer);
    
    // Global modifiers and force fields
    void addGlobalModifier(std::unique_ptr<ParticleModifier> modifier);
    void addGlobalForceField(std::unique_ptr<ParticleForceField> field);
    void clearGlobalModifiers();
    void clearGlobalForceFields();
    
    // Pool management
    ParticlePool& getPool() { return *pool; }
    
    // Query
    size_t getEmitterCount() const { return emitters.size(); }
    size_t getTotalParticleCount() const;
    
    // Configuration
    void setSimulationSpeed(float speed) { simulationSpeed = speed; }
    void setMaxParticles(size_t max);
    
private:
    std::unique_ptr<ParticlePool> pool;
    std::vector<std::unique_ptr<AdvancedParticleEmitter>> emitters;
    
    std::vector<std::unique_ptr<ParticleModifier>> globalModifiers;
    std::vector<std::unique_ptr<ParticleForceField>> globalForceFields;
    
    float simulationSpeed;
};

// Particle system effect presets
namespace ParticleEffects {
    
    AdvancedParticleEmitter* createFireEffect(AdvancedParticleSystem& system,
                                             const JJM::Math::Vector2D& position);
    
    AdvancedParticleEmitter* createSmokeEffect(AdvancedParticleSystem& system,
                                              const JJM::Math::Vector2D& position);
    
    AdvancedParticleEmitter* createExplosionEffect(AdvancedParticleSystem& system,
                                                   const JJM::Math::Vector2D& position);
    
    AdvancedParticleEmitter* createSparkEffect(AdvancedParticleSystem& system,
                                              const JJM::Math::Vector2D& position);
    
    AdvancedParticleEmitter* createMagicEffect(AdvancedParticleSystem& system,
                                              const JJM::Math::Vector2D& position);
    
    AdvancedParticleEmitter* createRainEffect(AdvancedParticleSystem& system,
                                             const JJM::Math::Vector2D& position,
                                             const JJM::Math::Vector2D& size);
    
} // namespace ParticleEffects

} // namespace Particles
} // namespace JJM

#endif // ADVANCED_PARTICLE_SYSTEM_H
