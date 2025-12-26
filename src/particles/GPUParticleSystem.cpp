#include "particles/GPUParticleSystem.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace Engine {

GPUParticleEmitter::GPUParticleEmitter()
    : m_posX(0.0f)
    , m_posY(0.0f)
    , m_posZ(0.0f)
    , m_isEmitting(false)
    , m_emissionTimer(0.0f)
    , m_activeParticles(0)
    , m_forceX(0.0f)
    , m_forceY(0.0f)
    , m_forceZ(0.0f)
    , m_hasVortex(false)
    , m_vortexX(0.0f)
    , m_vortexY(0.0f)
    , m_vortexZ(0.0f)
    , m_vortexStrength(0.0f)
    , m_vortexRadius(1.0f)
    , m_turbulenceStrength(0.0f)
    , m_turbulenceFrequency(1.0f)
    , m_collisionEnabled(false)
    , m_collisionDamping(0.5f)
    , m_collisionBounce(0.5f)
    , m_blendMode(0)
    , m_sortParticles(false)
    , m_vertexBuffer(0)
    , m_vertexArray(0)
    , m_computeShader(0)
    , m_renderShader(0)
    , m_texture(0)
{
    // Default settings
    m_settings.shape = ParticleEmitterShape::Point;
    m_settings.simulationSpace = ParticleSimulationSpace::World;
    m_settings.emissionRate = 10.0f;
    m_settings.maxParticles = 1000;
    m_settings.lifetime = 5.0f;
    m_settings.lifetimeVariation = 1.0f;
    m_settings.radius = 1.0f;
    m_settings.velocityX = 0.0f;
    m_settings.velocityY = 1.0f;
    m_settings.velocityZ = 0.0f;
    m_settings.velocityVariation = 0.5f;
    m_settings.startSize = 1.0f;
    m_settings.sizeVariation = 0.2f;
    m_settings.startColorR = 1.0f;
    m_settings.startColorG = 1.0f;
    m_settings.startColorB = 1.0f;
    m_settings.startColorA = 1.0f;
    m_settings.gravityX = 0.0f;
    m_settings.gravityY = -9.8f;
    m_settings.gravityZ = 0.0f;
    m_settings.damping = 0.99f;
    m_settings.startRotation = 0.0f;
    m_settings.rotationSpeed = 0.0f;
    m_settings.rotationVariation = 0.0f;
    
    initializeGPUBuffers();
}

GPUParticleEmitter::~GPUParticleEmitter() {
    // Cleanup GPU resources
}

void GPUParticleEmitter::setSettings(const ParticleEmitterSettings& settings) {
    m_settings = settings;
    m_particles.resize(settings.maxParticles);
}

void GPUParticleEmitter::setPosition(float x, float y, float z) {
    m_posX = x;
    m_posY = y;
    m_posZ = z;
}

void GPUParticleEmitter::getPosition(float& x, float& y, float& z) const {
    x = m_posX;
    y = m_posY;
    z = m_posZ;
}

void GPUParticleEmitter::emit(int count) {
    for (int i = 0; i < count && m_activeParticles < m_settings.maxParticles; ++i) {
        emitParticle();
    }
}

void GPUParticleEmitter::burst(int count) {
    emit(count);
}

void GPUParticleEmitter::setColorOverLifetime(const ParticleGradient& gradient) {
    m_colorGradient = gradient;
}

void GPUParticleEmitter::setSizeOverLifetime(const std::vector<float>& times, const std::vector<float>& sizes) {
    m_sizeTimeline = times;
    m_sizeCurve = sizes;
}

void GPUParticleEmitter::addConstantForce(float fx, float fy, float fz) {
    m_forceX += fx;
    m_forceY += fy;
    m_forceZ += fz;
}

void GPUParticleEmitter::addVortexForce(float x, float y, float z, float strength, float radius) {
    m_hasVortex = true;
    m_vortexX = x;
    m_vortexY = y;
    m_vortexZ = z;
    m_vortexStrength = strength;
    m_vortexRadius = radius;
}

void GPUParticleEmitter::addTurbulence(float strength, float frequency) {
    m_turbulenceStrength = strength;
    m_turbulenceFrequency = frequency;
}

void GPUParticleEmitter::setTexture(const std::string& texturePath) {
    m_texturePath = texturePath;
    // Load texture
}

void GPUParticleEmitter::update(float deltaTime) {
    if (!m_isEmitting) {
        return;
    }
    
    // Emit new particles
    m_emissionTimer += deltaTime;
    float emissionInterval = 1.0f / m_settings.emissionRate;
    
    while (m_emissionTimer >= emissionInterval && m_activeParticles < m_settings.maxParticles) {
        emitParticle();
        m_emissionTimer -= emissionInterval;
    }
    
    // Update existing particles
    for (int i = 0; i < m_activeParticles; ++i) {
        Particle& p = m_particles[i];
        
        p.age += deltaTime;
        
        // Remove dead particles
        if (p.age >= p.lifetime) {
            // Swap with last particle
            m_particles[i] = m_particles[m_activeParticles - 1];
            m_activeParticles--;
            i--;
            continue;
        }
        
        // Apply gravity
        p.vx += m_settings.gravityX * deltaTime;
        p.vy += m_settings.gravityY * deltaTime;
        p.vz += m_settings.gravityZ * deltaTime;
        
        // Apply constant forces
        p.vx += m_forceX * deltaTime;
        p.vy += m_forceY * deltaTime;
        p.vz += m_forceZ * deltaTime;
        
        // Apply vortex force
        if (m_hasVortex) {
            float dx = p.x - m_vortexX;
            float dy = p.y - m_vortexY;
            float dz = p.z - m_vortexZ;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            
            if (dist < m_vortexRadius && dist > 0.001f) {
                float strength = m_vortexStrength * (1.0f - dist / m_vortexRadius);
                
                // Tangential force
                p.vx += (-dy / dist) * strength * deltaTime;
                p.vy += (dx / dist) * strength * deltaTime;
            }
        }
        
        // Apply damping
        p.vx *= m_settings.damping;
        p.vy *= m_settings.damping;
        p.vz *= m_settings.damping;
        
        // Update position
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;
        p.z += p.vz * deltaTime;
        
        // Update rotation
        p.rotation += m_settings.rotationSpeed * deltaTime;
        
        // Update color over lifetime
        if (!m_colorGradient.times.empty()) {
            float t = p.age / p.lifetime;
            
            // Find color in gradient
            for (size_t j = 0; j < m_colorGradient.times.size() - 1; ++j) {
                if (t >= m_colorGradient.times[j] && t <= m_colorGradient.times[j + 1]) {
                    float localT = (t - m_colorGradient.times[j]) / 
                                  (m_colorGradient.times[j + 1] - m_colorGradient.times[j]);
                    
                    p.r = m_colorGradient.valuesR[j] + localT * (m_colorGradient.valuesR[j + 1] - m_colorGradient.valuesR[j]);
                    p.g = m_colorGradient.valuesG[j] + localT * (m_colorGradient.valuesG[j + 1] - m_colorGradient.valuesG[j]);
                    p.b = m_colorGradient.valuesB[j] + localT * (m_colorGradient.valuesB[j + 1] - m_colorGradient.valuesB[j]);
                    p.a = m_colorGradient.valuesA[j] + localT * (m_colorGradient.valuesA[j + 1] - m_colorGradient.valuesA[j]);
                    break;
                }
            }
        }
        
        // Update size over lifetime
        if (!m_sizeTimeline.empty()) {
            float t = p.age / p.lifetime;
            
            for (size_t j = 0; j < m_sizeTimeline.size() - 1; ++j) {
                if (t >= m_sizeTimeline[j] && t <= m_sizeTimeline[j + 1]) {
                    float localT = (t - m_sizeTimeline[j]) / (m_sizeTimeline[j + 1] - m_sizeTimeline[j]);
                    p.size = m_sizeCurve[j] + localT * (m_sizeCurve[j + 1] - m_sizeCurve[j]);
                    break;
                }
            }
        }
        
        // Collision detection
        if (m_collisionEnabled && p.y < 0.0f) {
            p.y = 0.0f;
            p.vy = -p.vy * m_collisionBounce;
            p.vx *= m_collisionDamping;
            p.vz *= m_collisionDamping;
        }
    }
    
    updateGPUBuffers();
}

void GPUParticleEmitter::render() {
    // Render particles using GPU
}

void GPUParticleEmitter::initializeGPUBuffers() {
    // Initialize OpenGL/Vulkan buffers
}

void GPUParticleEmitter::updateGPUBuffers() {
    // Upload particle data to GPU
}

void GPUParticleEmitter::emitParticle() {
    if (m_activeParticles >= m_settings.maxParticles) {
        return;
    }
    
    Particle& p = m_particles[m_activeParticles++];
    
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(-1.0, 1.0);
    
    // Initial position based on emitter shape
    switch (m_settings.shape) {
        case ParticleEmitterShape::Point:
            p.x = m_posX;
            p.y = m_posY;
            p.z = m_posZ;
            break;
            
        case ParticleEmitterShape::Sphere: {
            float theta = dis(gen) * 3.14159f;
            float phi = dis(gen) * 3.14159f * 2.0f;
            float r = m_settings.radius * std::pow(dis(gen) * 0.5f + 0.5f, 1.0f / 3.0f);
            
            p.x = m_posX + r * std::sin(theta) * std::cos(phi);
            p.y = m_posY + r * std::sin(theta) * std::sin(phi);
            p.z = m_posZ + r * std::cos(theta);
            break;
        }
        
        case ParticleEmitterShape::Box:
            p.x = m_posX + dis(gen) * m_settings.boxWidth * 0.5f;
            p.y = m_posY + dis(gen) * m_settings.boxHeight * 0.5f;
            p.z = m_posZ + dis(gen) * m_settings.boxDepth * 0.5f;
            break;
            
        case ParticleEmitterShape::Cone: {
            float angle = dis(gen) * m_settings.coneAngle * 3.14159f / 180.0f;
            float rotation = dis(gen) * 3.14159f * 2.0f;
            float distance = dis(gen) * 0.5f + 0.5f;
            
            p.x = m_posX + distance * std::sin(angle) * std::cos(rotation);
            p.y = m_posY + distance * std::cos(angle);
            p.z = m_posZ + distance * std::sin(angle) * std::sin(rotation);
            break;
        }
        
        default:
            p.x = m_posX;
            p.y = m_posY;
            p.z = m_posZ;
            break;
    }
    
    // Initial velocity
    float velVar = m_settings.velocityVariation;
    p.vx = m_settings.velocityX + dis(gen) * velVar;
    p.vy = m_settings.velocityY + dis(gen) * velVar;
    p.vz = m_settings.velocityZ + dis(gen) * velVar;
    
    // Initial color
    p.r = m_settings.startColorR;
    p.g = m_settings.startColorG;
    p.b = m_settings.startColorB;
    p.a = m_settings.startColorA;
    
    // Initial size
    p.size = m_settings.startSize + dis(gen) * m_settings.sizeVariation;
    
    // Initial rotation
    p.rotation = m_settings.startRotation + dis(gen) * m_settings.rotationVariation;
    
    // Lifetime
    p.lifetime = m_settings.lifetime + dis(gen) * m_settings.lifetimeVariation;
    p.age = 0.0f;
}

void GPUParticleEmitter::applyForces(float deltaTime) {
    // Additional force application if needed
}

// GPUParticleSystem implementation
GPUParticleSystem::GPUParticleSystem()
    : m_nextEmitterId(1)
    , m_globalGravityX(0.0f)
    , m_globalGravityY(-9.8f)
    , m_globalGravityZ(0.0f)
    , m_globalMaxParticles(10000)
{
}

GPUParticleSystem& GPUParticleSystem::getInstance() {
    static GPUParticleSystem instance;
    return instance;
}

void GPUParticleSystem::initialize() {
    // Initialize GPU particle system
}

void GPUParticleSystem::shutdown() {
    for (auto* emitter : m_emitters) {
        delete emitter;
    }
    m_emitters.clear();
}

void GPUParticleSystem::update(float deltaTime) {
    for (auto* emitter : m_emitters) {
        if (emitter) {
            emitter->update(deltaTime);
        }
    }
}

void GPUParticleSystem::render() {
    for (auto* emitter : m_emitters) {
        if (emitter) {
            emitter->render();
        }
    }
}

int GPUParticleSystem::createEmitter(const std::string& name) {
    GPUParticleEmitter* emitter = new GPUParticleEmitter();
    m_emitters.push_back(emitter);
    return m_nextEmitterId++;
}

void GPUParticleSystem::destroyEmitter(int emitterId) {
    if (emitterId > 0 && emitterId <= static_cast<int>(m_emitters.size())) {
        delete m_emitters[emitterId - 1];
        m_emitters[emitterId - 1] = nullptr;
    }
}

GPUParticleEmitter* GPUParticleSystem::getEmitter(int emitterId) {
    if (emitterId > 0 && emitterId <= static_cast<int>(m_emitters.size())) {
        return m_emitters[emitterId - 1];
    }
    return nullptr;
}

int GPUParticleSystem::createFireEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Cone;
        settings.coneAngle = 20.0f;
        settings.emissionRate = 50.0f;
        settings.lifetime = 2.0f;
        settings.velocityY = 5.0f;
        settings.velocityVariation = 2.0f;
        settings.startColorR = 1.0f;
        settings.startColorG = 0.5f;
        settings.startColorB = 0.1f;
        settings.gravityY = 0.0f;
        emitter->setSettings(settings);
    }
    
    return id;
}

int GPUParticleSystem::createSmokeEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Sphere;
        settings.radius = 0.5f;
        settings.emissionRate = 20.0f;
        settings.lifetime = 5.0f;
        settings.velocityY = 1.0f;
        settings.startColorR = 0.5f;
        settings.startColorG = 0.5f;
        settings.startColorB = 0.5f;
        settings.startColorA = 0.5f;
        settings.startSize = 2.0f;
        settings.gravityY = 0.0f;
        settings.damping = 0.95f;
        emitter->setSettings(settings);
    }
    
    return id;
}

int GPUParticleSystem::createExplosionEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Sphere;
        settings.radius = 0.1f;
        settings.emissionRate = 0.0f;
        settings.lifetime = 1.0f;
        settings.velocityVariation = 10.0f;
        settings.startSize = 0.5f;
        emitter->setSettings(settings);
        
        emitter->burst(100);
        emitter->setEmitting(false);
    }
    
    return id;
}

int GPUParticleSystem::createSparkEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Point;
        settings.emissionRate = 100.0f;
        settings.lifetime = 0.5f;
        settings.velocityVariation = 5.0f;
        settings.startSize = 0.1f;
        settings.startColorR = 1.0f;
        settings.startColorG = 0.8f;
        settings.startColorB = 0.0f;
        emitter->setSettings(settings);
    }
    
    return id;
}

int GPUParticleSystem::createRainEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Box;
        settings.boxWidth = 50.0f;
        settings.boxHeight = 0.1f;
        settings.boxDepth = 50.0f;
        settings.emissionRate = 500.0f;
        settings.lifetime = 5.0f;
        settings.velocityY = -20.0f;
        settings.startSize = 0.1f;
        settings.gravityY = -20.0f;
        emitter->setSettings(settings);
    }
    
    return id;
}

int GPUParticleSystem::createSnowEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Box;
        settings.boxWidth = 50.0f;
        settings.boxHeight = 0.1f;
        settings.boxDepth = 50.0f;
        settings.emissionRate = 100.0f;
        settings.lifetime = 10.0f;
        settings.velocityY = -1.0f;
        settings.startSize = 0.5f;
        settings.gravityY = 0.0f;
        settings.damping = 0.99f;
        emitter->setSettings(settings);
    }
    
    return id;
}

int GPUParticleSystem::createMagicEmitter(const std::string& name) {
    int id = createEmitter(name);
    GPUParticleEmitter* emitter = getEmitter(id);
    
    if (emitter) {
        ParticleEmitterSettings settings = emitter->getSettings();
        settings.shape = ParticleEmitterShape::Sphere;
        settings.radius = 2.0f;
        settings.emissionRate = 50.0f;
        settings.lifetime = 3.0f;
        settings.velocityVariation = 1.0f;
        settings.startColorR = 0.5f;
        settings.startColorG = 0.0f;
        settings.startColorB = 1.0f;
        settings.gravityY = 0.0f;
        emitter->setSettings(settings);
        
        emitter->addVortexForce(0.0f, 0.0f, 0.0f, 5.0f, 5.0f);
    }
    
    return id;
}

void GPUParticleSystem::setGlobalGravity(float x, float y, float z) {
    m_globalGravityX = x;
    m_globalGravityY = y;
    m_globalGravityZ = z;
}

int GPUParticleSystem::getEmitterCount() const {
    int count = 0;
    for (const auto* emitter : m_emitters) {
        if (emitter != nullptr) {
            count++;
        }
    }
    return count;
}

int GPUParticleSystem::getTotalParticleCount() const {
    int total = 0;
    for (const auto* emitter : m_emitters) {
        if (emitter != nullptr) {
            total += emitter->getParticleCount();
        }
    }
    return total;
}

} // namespace Engine
