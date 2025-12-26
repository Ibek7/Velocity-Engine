#include "physics/ClothSimulation.h"
#include <cmath>

namespace Engine {

// Cloth implementation
Cloth::Cloth(int width, int height, float spacing)
    : m_width(width)
    , m_height(height)
    , m_spacing(spacing)
    , m_stiffness(1.0f)
    , m_damping(0.01f)
    , m_gravityX(0.0f)
    , m_gravityY(-9.81f)
    , m_gravityZ(0.0f)
    , m_windX(0.0f)
    , m_windY(0.0f)
    , m_windZ(0.0f)
    , m_tearingEnabled(false)
    , m_tearThreshold(2.0f)
{
    // Create particles in grid
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            ClothParticle particle;
            particle.x = x * spacing;
            particle.y = 0.0f;
            particle.z = y * spacing;
            particle.oldX = particle.x;
            particle.oldY = particle.y;
            particle.oldZ = particle.z;
            particle.mass = 1.0f;
            particle.isPinned = false;
            
            m_particles.push_back(particle);
        }
    }
    
    // Create structural constraints (horizontal and vertical)
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = getParticleIndex(x, y);
            
            // Right neighbor
            if (x < width - 1) {
                ClothConstraint constraint;
                constraint.particleA = idx;
                constraint.particleB = getParticleIndex(x + 1, y);
                constraint.restLength = spacing;
                constraint.stiffness = m_stiffness;
                m_constraints.push_back(constraint);
            }
            
            // Bottom neighbor
            if (y < height - 1) {
                ClothConstraint constraint;
                constraint.particleA = idx;
                constraint.particleB = getParticleIndex(x, y + 1);
                constraint.restLength = spacing;
                constraint.stiffness = m_stiffness;
                m_constraints.push_back(constraint);
            }
            
            // Diagonal constraints for shear resistance
            if (x < width - 1 && y < height - 1) {
                ClothConstraint constraint;
                constraint.particleA = idx;
                constraint.particleB = getParticleIndex(x + 1, y + 1);
                constraint.restLength = spacing * std::sqrt(2.0f);
                constraint.stiffness = m_stiffness;
                m_constraints.push_back(constraint);
            }
            
            if (x > 0 && y < height - 1) {
                ClothConstraint constraint;
                constraint.particleA = idx;
                constraint.particleB = getParticleIndex(x - 1, y + 1);
                constraint.restLength = spacing * std::sqrt(2.0f);
                constraint.stiffness = m_stiffness;
                m_constraints.push_back(constraint);
            }
        }
    }
}

Cloth::~Cloth() {
}

void Cloth::pinParticle(int index) {
    if (index >= 0 && index < static_cast<int>(m_particles.size())) {
        m_particles[index].isPinned = true;
    }
}

void Cloth::unpinParticle(int index) {
    if (index >= 0 && index < static_cast<int>(m_particles.size())) {
        m_particles[index].isPinned = false;
    }
}

void Cloth::setGravity(float x, float y, float z) {
    m_gravityX = x;
    m_gravityY = y;
    m_gravityZ = z;
}

void Cloth::setWind(float x, float y, float z) {
    m_windX = x;
    m_windY = y;
    m_windZ = z;
}

void Cloth::applyForce(int particleIndex, float x, float y, float z) {
    if (particleIndex >= 0 && particleIndex < static_cast<int>(m_particles.size())) {
        ClothParticle& p = m_particles[particleIndex];
        if (!p.isPinned) {
            p.x += x / p.mass;
            p.y += y / p.mass;
            p.z += z / p.mass;
        }
    }
}

void Cloth::update(float deltaTime) {
    integrateVerlet(deltaTime);
    satisfyConstraints();
    handleCollisions();
    applyWind();
}

void Cloth::reset() {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            int idx = getParticleIndex(x, y);
            ClothParticle& p = m_particles[idx];
            p.x = x * m_spacing;
            p.y = 0.0f;
            p.z = y * m_spacing;
            p.oldX = p.x;
            p.oldY = p.y;
            p.oldZ = p.z;
        }
    }
}

void Cloth::addSphereCollider(float x, float y, float z, float radius) {
    SphereCollider collider;
    collider.x = x;
    collider.y = y;
    collider.z = z;
    collider.radius = radius;
    m_colliders.push_back(collider);
}

void Cloth::clearColliders() {
    m_colliders.clear();
}

void Cloth::getParticlePosition(int index, float& x, float& y, float& z) const {
    if (index >= 0 && index < static_cast<int>(m_particles.size())) {
        const ClothParticle& p = m_particles[index];
        x = p.x;
        y = p.y;
        z = p.z;
    }
}

void Cloth::integrateVerlet(float deltaTime) {
    for (auto& p : m_particles) {
        if (p.isPinned) {
            continue;
        }
        
        // Store current position
        float tempX = p.x;
        float tempY = p.y;
        float tempZ = p.z;
        
        // Verlet integration
        float velocityX = (p.x - p.oldX) * (1.0f - m_damping);
        float velocityY = (p.y - p.oldY) * (1.0f - m_damping);
        float velocityZ = (p.z - p.oldZ) * (1.0f - m_damping);
        
        // Apply gravity
        velocityY += m_gravityY * deltaTime;
        
        // Update position
        p.x += velocityX;
        p.y += velocityY;
        p.z += velocityZ;
        
        // Store old position
        p.oldX = tempX;
        p.oldY = tempY;
        p.oldZ = tempZ;
    }
}

void Cloth::satisfyConstraints() {
    for (auto& constraint : m_constraints) {
        ClothParticle& pA = m_particles[constraint.particleA];
        ClothParticle& pB = m_particles[constraint.particleB];
        
        if (pA.isPinned && pB.isPinned) {
            continue;
        }
        
        // Calculate distance
        float dx = pB.x - pA.x;
        float dy = pB.y - pA.y;
        float dz = pB.z - pA.z;
        float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        
        if (distance == 0.0f) {
            continue;
        }
        
        // Check for tearing
        if (m_tearingEnabled && distance > constraint.restLength * m_tearThreshold) {
            // Mark constraint for removal
            continue;
        }
        
        // Calculate correction
        float difference = (constraint.restLength - distance) / distance;
        float correctionX = dx * difference * 0.5f * constraint.stiffness;
        float correctionY = dy * difference * 0.5f * constraint.stiffness;
        float correctionZ = dz * difference * 0.5f * constraint.stiffness;
        
        // Apply correction
        if (!pA.isPinned) {
            pA.x -= correctionX;
            pA.y -= correctionY;
            pA.z -= correctionZ;
        }
        
        if (!pB.isPinned) {
            pB.x += correctionX;
            pB.y += correctionY;
            pB.z += correctionZ;
        }
    }
}

void Cloth::handleCollisions() {
    for (auto& p : m_particles) {
        if (p.isPinned) {
            continue;
        }
        
        // Check sphere colliders
        for (const auto& collider : m_colliders) {
            float dx = p.x - collider.x;
            float dy = p.y - collider.y;
            float dz = p.z - collider.z;
            float distSquared = dx * dx + dy * dy + dz * dz;
            float radiusSquared = collider.radius * collider.radius;
            
            if (distSquared < radiusSquared) {
                // Push particle out of sphere
                float dist = std::sqrt(distSquared);
                if (dist > 0.0f) {
                    float correction = (collider.radius - dist) / dist;
                    p.x += dx * correction;
                    p.y += dy * correction;
                    p.z += dz * correction;
                }
            }
        }
    }
}

void Cloth::applyWind() {
    // TODO: Apply wind force to cloth triangles based on normal
}

// ClothSystem implementation
ClothSystem::ClothSystem()
    : m_substeps(5)
    , m_constraintIterations(3)
    , m_selfCollisionEnabled(false)
{
}

ClothSystem& ClothSystem::getInstance() {
    static ClothSystem instance;
    return instance;
}

Cloth* ClothSystem::createCloth(int id, int width, int height, float spacing) {
    Cloth* cloth = new Cloth(width, height, spacing);
    m_cloths[id] = cloth;
    return cloth;
}

Cloth* ClothSystem::getCloth(int id) {
    auto it = m_cloths.find(id);
    if (it != m_cloths.end()) {
        return it->second;
    }
    return nullptr;
}

void ClothSystem::removeCloth(int id) {
    auto it = m_cloths.find(id);
    if (it != m_cloths.end()) {
        delete it->second;
        m_cloths.erase(it);
    }
}

void ClothSystem::update(float deltaTime) {
    float substepDelta = deltaTime / m_substeps;
    
    for (int i = 0; i < m_substeps; ++i) {
        for (auto& pair : m_cloths) {
            pair.second->update(substepDelta);
            
            // Apply constraint iterations
            for (int j = 0; j < m_constraintIterations; ++j) {
                // Additional constraint passes
            }
        }
    }
}

} // namespace Engine
