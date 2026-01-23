#include "particles/ParticleCollision.h"
#include <cmath>
#include <algorithm>

namespace Engine {

// Vector3 Implementation
float Vector3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3 Vector3::normalized() const {
    float len = length();
    return len > 0.0f ? Vector3(x / len, y / len, z / len) : Vector3();
}

// ParticleCollisionSystem Implementation
ParticleCollisionSystem::ParticleCollisionSystem()
    : m_particleCollisionsEnabled(false)
    , m_spatialPartitioningEnabled(true)
    , m_gridCellSize(1.0f)
    , m_collisionTests(0)
    , m_collisionCount(0) {
}

ParticleCollisionSystem::~ParticleCollisionSystem() {
}

int ParticleCollisionSystem::addPlane(const CollisionPlane& plane) {
    m_planes.push_back(plane);
    return static_cast<int>(m_planes.size() - 1);
}

int ParticleCollisionSystem::addSphere(const CollisionSphere& sphere) {
    m_spheres.push_back(sphere);
    return static_cast<int>(m_spheres.size() - 1);
}

int ParticleCollisionSystem::addBox(const CollisionBox& box) {
    m_boxes.push_back(box);
    return static_cast<int>(m_boxes.size() - 1);
}

void ParticleCollisionSystem::removeCollider(int type, int id) {
    if (type == 0 && id >= 0 && id < static_cast<int>(m_planes.size())) {
        m_planes.erase(m_planes.begin() + id);
    } else if (type == 1 && id >= 0 && id < static_cast<int>(m_spheres.size())) {
        m_spheres.erase(m_spheres.begin() + id);
    } else if (type == 2 && id >= 0 && id < static_cast<int>(m_boxes.size())) {
        m_boxes.erase(m_boxes.begin() + id);
    }
}

void ParticleCollisionSystem::clearColliders() {
    m_planes.clear();
    m_spheres.clear();
    m_boxes.clear();
}

void ParticleCollisionSystem::resolveCollisions(Particle* particles, int count, float deltaTime) {
    if (!particles || count == 0) return;
    
    m_collisionTests = 0;
    m_collisionCount = 0;
    
    // Check collisions with world geometry
    for (int i = 0; i < count; ++i) {
        if (!particles[i].active) continue;
        
        // Plane collisions
        for (const auto& plane : m_planes) {
            if (checkPlaneCollision(particles[i], plane, deltaTime)) {
                m_collisionCount++;
            }
            m_collisionTests++;
        }
        
        // Sphere collisions
        for (const auto& sphere : m_spheres) {
            if (checkSphereCollision(particles[i], sphere, deltaTime)) {
                m_collisionCount++;
            }
            m_collisionTests++;
        }
        
        // Box collisions
        for (const auto& box : m_boxes) {
            if (checkBoxCollision(particles[i], box, deltaTime)) {
                m_collisionCount++;
            }
            m_collisionTests++;
        }
    }
    
    // Particle-particle collisions
    if (m_particleCollisionsEnabled) {
        for (int i = 0; i < count; ++i) {
            if (!particles[i].active) continue;
            
            for (int j = i + 1; j < count; ++j) {
                if (!particles[j].active) continue;
                if (!canCollide(particles[i].groupId, particles[j].groupId)) continue;
                
                if (checkParticleCollision(particles[i], particles[j], deltaTime)) {
                    m_collisionCount++;
                }
                m_collisionTests++;
            }
        }
    }
}

void ParticleCollisionSystem::setCollisionLayers(int groupId, unsigned int layers) {
    if (groupId >= static_cast<int>(m_collisionLayers.size())) {
        m_collisionLayers.resize(groupId + 1, CollisionLayer::Layer_All);
    }
    m_collisionLayers[groupId] = layers;
}

bool ParticleCollisionSystem::canCollide(int groupA, int groupB) const {
    if (groupA >= static_cast<int>(m_collisionLayers.size()) || 
        groupB >= static_cast<int>(m_collisionLayers.size())) {
        return true;
    }
    
    return (m_collisionLayers[groupA] & m_collisionLayers[groupB]) != 0;
}

void ParticleCollisionSystem::setCollisionCallback(std::function<void(int, int, const Vector3&)> callback) {
    m_collisionCallback = callback;
}

void ParticleCollisionSystem::getStatistics(int& outTests, int& outCollisions) const {
    outTests = m_collisionTests;
    outCollisions = m_collisionCount;
}

void ParticleCollisionSystem::resetStatistics() {
    m_collisionTests = 0;
    m_collisionCount = 0;
}

bool ParticleCollisionSystem::checkPlaneCollision(Particle& particle, const CollisionPlane& plane, float deltaTime) {
    float dist = particle.position.dot(plane.normal) - plane.distance;
    
    if (dist < particle.radius) {
        // Collision detected
        particle.position = particle.position - plane.normal * (dist - particle.radius);
        
        // Reflect velocity
        float vn = particle.velocity.dot(plane.normal);
        if (vn < 0.0f) {
            particle.velocity = particle.velocity - plane.normal * (vn * (1.0f + plane.restitution));
            
            // Apply friction
            Vector3 tangent = particle.velocity - plane.normal * particle.velocity.dot(plane.normal);
            particle.velocity = particle.velocity - tangent * plane.friction;
        }
        
        return true;
    }
    
    return false;
}

bool ParticleCollisionSystem::checkSphereCollision(Particle& particle, const CollisionSphere& sphere, float deltaTime) {
    Vector3 diff = particle.position - sphere.center;
    float distSq = diff.dot(diff);
    float minDist = particle.radius + sphere.radius;
    
    if (distSq < minDist * minDist && distSq > 0.0001f) {
        float dist = std::sqrt(distSq);
        Vector3 normal = diff * (1.0f / dist);
        
        // Push particle out
        particle.position = sphere.center + normal * minDist;
        
        // Reflect velocity
        float vn = particle.velocity.dot(normal);
        if (vn < 0.0f) {
            particle.velocity = particle.velocity - normal * (vn * (1.0f + sphere.restitution));
        }
        
        return true;
    }
    
    return false;
}

bool ParticleCollisionSystem::checkBoxCollision(Particle& particle, const CollisionBox& box, float deltaTime) {
    Vector3 closest(
        std::max(box.min.x, std::min(particle.position.x, box.max.x)),
        std::max(box.min.y, std::min(particle.position.y, box.max.y)),
        std::max(box.min.z, std::min(particle.position.z, box.max.z))
    );
    
    Vector3 diff = particle.position - closest;
    float distSq = diff.dot(diff);
    
    if (distSq < particle.radius * particle.radius && distSq > 0.0001f) {
        float dist = std::sqrt(distSq);
        Vector3 normal = diff * (1.0f / dist);
        
        // Push particle out
        particle.position = closest + normal * particle.radius;
        
        // Reflect velocity
        float vn = particle.velocity.dot(normal);
        if (vn < 0.0f) {
            particle.velocity = particle.velocity - normal * (vn * (1.0f + box.restitution));
        }
        
        return true;
    }
    
    return false;
}

bool ParticleCollisionSystem::checkParticleCollision(Particle& p1, Particle& p2, float deltaTime) {
    Vector3 diff = p2.position - p1.position;
    float distSq = diff.dot(diff);
    float minDist = p1.radius + p2.radius;
    
    if (distSq < minDist * minDist && distSq > 0.0001f) {
        float dist = std::sqrt(distSq);
        Vector3 normal = diff * (1.0f / dist);
        
        resolveParticleCollision(p1, p2, normal);
        
        if (m_collisionCallback) {
            m_collisionCallback(0, 0, p1.position);
        }
        
        return true;
    }
    
    return false;
}

void ParticleCollisionSystem::resolveParticleCollision(Particle& p1, Particle& p2, const Vector3& normal) {
    // Calculate relative velocity
    Vector3 relVel = p2.velocity - p1.velocity;
    float velAlongNormal = relVel.dot(normal);
    
    if (velAlongNormal > 0) return;
    
    // Calculate restitution
    float e = std::min(p1.restitution, p2.restitution);
    
    // Calculate impulse scalar
    float j = -(1 + e) * velAlongNormal;
    j /= (1.0f / p1.mass + 1.0f / p2.mass);
    
    // Apply impulse
    Vector3 impulse = normal * j;
    p1.velocity = p1.velocity - impulse * (1.0f / p1.mass);
    p2.velocity = p2.velocity + impulse * (1.0f / p2.mass);
    
    // Separate particles
    float overlap = (p1.radius + p2.radius) - (p2.position - p1.position).length();
    if (overlap > 0) {
        Vector3 separation = normal * (overlap * 0.5f);
        p1.position = p1.position - separation;
        p2.position = p2.position + separation;
    }
}

void ParticleCollisionSystem::buildSpatialGrid(Particle* particles, int count) {
    // Spatial grid implementation for optimization
    // This would be implemented for better performance with many particles
}

// ParticleConstraint Implementation
ParticleConstraint::ParticleConstraint(int p1, int p2, float restLength, float stiffness)
    : m_particle1(p1)
    , m_particle2(p2)
    , m_restLength(restLength)
    , m_stiffness(stiffness) {
}

void ParticleConstraint::solve(Particle* particles) {
    Vector3 delta = particles[m_particle2].position - particles[m_particle1].position;
    float currentLength = delta.length();
    
    if (currentLength < 0.0001f) return;
    
    float diff = (currentLength - m_restLength) / currentLength;
    Vector3 correction = delta * (diff * 0.5f * m_stiffness);
    
    particles[m_particle1].position = particles[m_particle1].position + correction;
    particles[m_particle2].position = particles[m_particle2].position - correction;
}

// ParticleForceField Implementation
ParticleForceField::ParticleForceField(Type type)
    : m_type(type)
    , m_strength(1.0f)
    , m_radius(10.0f)
    , m_falloff(1.0f)
    , m_noiseScale(1.0f) {
}

void ParticleForceField::apply(Particle& particle, float deltaTime) {
    Vector3 force;
    
    switch (m_type) {
        case Type::Point: {
            Vector3 dir = m_position - particle.position;
            float dist = dir.length();
            
            if (dist < m_radius && dist > 0.0001f) {
                float falloff = 1.0f - std::pow(dist / m_radius, m_falloff);
                force = dir.normalized() * (m_strength * falloff);
            }
            break;
        }
        
        case Type::Directional:
            force = m_direction * m_strength;
            break;
        
        case Type::Vortex: {
            Vector3 toCenter = m_position - particle.position;
            float dist = toCenter.length();
            
            if (dist < m_radius && dist > 0.0001f) {
                Vector3 tangent(-toCenter.y, toCenter.x, 0.0f);
                tangent = tangent.normalized();
                
                float falloff = 1.0f - (dist / m_radius);
                force = tangent * (m_strength * falloff);
            }
            break;
        }
        
        case Type::Turbulence: {
            // Simple pseudo-random turbulence
            float noise = std::sin(particle.position.x * m_noiseScale) * 
                         std::cos(particle.position.y * m_noiseScale);
            force = Vector3(noise, -noise, 0.0f) * m_strength;
            break;
        }
    }
    
    particle.force = particle.force + force;
}

} // namespace Engine
