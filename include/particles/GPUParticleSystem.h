#pragma once

#include <vector>
#include <string>
#include <functional>

// GPU-accelerated particle system
namespace Engine {

enum class ParticleEmitterShape {
    Point,
    Sphere,
    Box,
    Cone,
    Mesh
};

enum class ParticleSimulationSpace {
    World,
    Local
};

struct ParticleEmitterSettings {
    ParticleEmitterShape shape;
    ParticleSimulationSpace simulationSpace;
    
    // Emission
    float emissionRate;
    int maxParticles;
    float lifetime;
    float lifetimeVariation;
    
    // Shape parameters
    float radius;
    float boxWidth, boxHeight, boxDepth;
    float coneAngle;
    
    // Initial velocity
    float velocityX, velocityY, velocityZ;
    float velocityVariation;
    
    // Initial size
    float startSize;
    float sizeVariation;
    
    // Initial color
    float startColorR, startColorG, startColorB, startColorA;
    
    // Gravity
    float gravityX, gravityY, gravityZ;
    
    // Damping
    float damping;
    
    // Rotation
    float startRotation;
    float rotationSpeed;
    float rotationVariation;
};

struct ParticleGradient {
    std::vector<float> times;
    std::vector<float> valuesR;
    std::vector<float> valuesG;
    std::vector<float> valuesB;
    std::vector<float> valuesA;
};

struct Particle {
    float x, y, z;
    float vx, vy, vz;
    float r, g, b, a;
    float size;
    float rotation;
    float lifetime;
    float age;
};

class GPUParticleEmitter {
public:
    GPUParticleEmitter();
    ~GPUParticleEmitter();

    // Configuration
    void setSettings(const ParticleEmitterSettings& settings);
    const ParticleEmitterSettings& getSettings() const { return m_settings; }
    
    // Position
    void setPosition(float x, float y, float z);
    void getPosition(float& x, float& y, float& z) const;
    
    // Emission control
    void setEmitting(bool emitting) { m_isEmitting = emitting; }
    bool isEmitting() const { return m_isEmitting; }
    void emit(int count);
    void burst(int count);
    
    // Color over lifetime
    void setColorOverLifetime(const ParticleGradient& gradient);
    void setSizeOverLifetime(const std::vector<float>& times, const std::vector<float>& sizes);
    
    // Forces
    void addConstantForce(float fx, float fy, float fz);
    void addVortexForce(float x, float y, float z, float strength, float radius);
    void addTurbulence(float strength, float frequency);
    
    // Collision
    void enableCollision(bool enable) { m_collisionEnabled = enable; }
    void setCollisionDamping(float damping) { m_collisionDamping = damping; }
    void setCollisionBounce(float bounce) { m_collisionBounce = bounce; }
    
    // Rendering
    void setTexture(const std::string& texturePath);
    void setBlendMode(int blendMode) { m_blendMode = blendMode; }
    void setSortParticles(bool sort) { m_sortParticles = sort; }
    
    // Update
    void update(float deltaTime);
    void render();
    
    // Query
    int getParticleCount() const { return m_activeParticles; }
    int getMaxParticles() const { return m_settings.maxParticles; }
    
    // GPU buffers
    unsigned int getVertexBuffer() const { return m_vertexBuffer; }
    unsigned int getComputeShader() const { return m_computeShader; }

private:
    void initializeGPUBuffers();
    void updateGPUBuffers();
    void emitParticle();
    void applyForces(float deltaTime);
    
    ParticleEmitterSettings m_settings;
    
    float m_posX, m_posY, m_posZ;
    bool m_isEmitting;
    float m_emissionTimer;
    
    std::vector<Particle> m_particles;
    int m_activeParticles;
    
    ParticleGradient m_colorGradient;
    std::vector<float> m_sizeTimeline;
    std::vector<float> m_sizeCurve;
    
    // Forces
    float m_forceX, m_forceY, m_forceZ;
    bool m_hasVortex;
    float m_vortexX, m_vortexY, m_vortexZ;
    float m_vortexStrength, m_vortexRadius;
    float m_turbulenceStrength, m_turbulenceFrequency;
    
    // Collision
    bool m_collisionEnabled;
    float m_collisionDamping;
    float m_collisionBounce;
    
    // Rendering
    std::string m_texturePath;
    int m_blendMode;
    bool m_sortParticles;
    
    // GPU resources
    unsigned int m_vertexBuffer;
    unsigned int m_vertexArray;
    unsigned int m_computeShader;
    unsigned int m_renderShader;
    unsigned int m_texture;
};

class GPUParticleSystem {
public:
    static GPUParticleSystem& getInstance();

    void initialize();
    void shutdown();
    void update(float deltaTime);
    void render();

    // Emitter management
    int createEmitter(const std::string& name);
    void destroyEmitter(int emitterId);
    GPUParticleEmitter* getEmitter(int emitterId);
    
    // Presets
    int createFireEmitter(const std::string& name);
    int createSmokeEmitter(const std::string& name);
    int createExplosionEmitter(const std::string& name);
    int createSparkEmitter(const std::string& name);
    int createRainEmitter(const std::string& name);
    int createSnowEmitter(const std::string& name);
    int createMagicEmitter(const std::string& name);
    
    // Global settings
    void setGlobalGravity(float x, float y, float z);
    void setMaxParticlesGlobal(int maxParticles) { m_globalMaxParticles = maxParticles; }
    
    // Query
    int getEmitterCount() const;
    int getTotalParticleCount() const;

private:
    GPUParticleSystem();
    GPUParticleSystem(const GPUParticleSystem&) = delete;
    GPUParticleSystem& operator=(const GPUParticleSystem&) = delete;

    std::vector<GPUParticleEmitter*> m_emitters;
    int m_nextEmitterId;
    
    float m_globalGravityX, m_globalGravityY, m_globalGravityZ;
    int m_globalMaxParticles;
};

} // namespace Engine
