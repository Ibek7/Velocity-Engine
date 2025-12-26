#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

// Cloth simulation system
namespace Engine {

struct ClothParticle {
    float x, y, z;
    float oldX, oldY, oldZ;
    float mass;
    bool isPinned;
};

struct ClothConstraint {
    int particleA;
    int particleB;
    float restLength;
    float stiffness;
};

class Cloth {
public:
    Cloth(int width, int height, float spacing);
    ~Cloth();

    // Setup
    void pinParticle(int index);
    void unpinParticle(int index);
    void setStiffness(float stiffness) { m_stiffness = stiffness; }
    void setDamping(float damping) { m_damping = damping; }
    
    // Forces
    void setGravity(float x, float y, float z);
    void setWind(float x, float y, float z);
    void applyForce(int particleIndex, float x, float y, float z);
    
    // Simulation
    void update(float deltaTime);
    void reset();
    
    // Collision
    void addSphereCollider(float x, float y, float z, float radius);
    void clearColliders();
    
    // Query
    const std::vector<ClothParticle>& getParticles() const { return m_particles; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void getParticlePosition(int index, float& x, float& y, float& z) const;
    
    // Tearing
    void enableTearing(bool enable) { m_tearingEnabled = enable; }
    void setTearThreshold(float threshold) { m_tearThreshold = threshold; }

private:
    void integrateVerlet(float deltaTime);
    void satisfyConstraints();
    void handleCollisions();
    void applyWind();
    
    int getParticleIndex(int x, int y) const { return y * m_width + x; }

    int m_width;
    int m_height;
    float m_spacing;
    float m_stiffness;
    float m_damping;
    
    std::vector<ClothParticle> m_particles;
    std::vector<ClothConstraint> m_constraints;
    
    float m_gravityX;
    float m_gravityY;
    float m_gravityZ;
    float m_windX;
    float m_windY;
    float m_windZ;
    
    bool m_tearingEnabled;
    float m_tearThreshold;
    
    struct SphereCollider {
        float x, y, z, radius;
    };
    std::vector<SphereCollider> m_colliders;
};

class ClothSystem {
public:
    static ClothSystem& getInstance();

    // Cloth management
    Cloth* createCloth(int id, int width, int height, float spacing);
    Cloth* getCloth(int id);
    void removeCloth(int id);
    
    // Update
    void update(float deltaTime);
    
    // Global settings
    void setSubsteps(int substeps) { m_substeps = substeps; }
    void setIterations(int iterations) { m_constraintIterations = iterations; }
    void enableSelfCollision(bool enable) { m_selfCollisionEnabled = enable; }

private:
    ClothSystem();
    ClothSystem(const ClothSystem&) = delete;
    ClothSystem& operator=(const ClothSystem&) = delete;

    std::map<int, Cloth*> m_cloths;
    int m_substeps;
    int m_constraintIterations;
    bool m_selfCollisionEnabled;
};

} // namespace Engine
