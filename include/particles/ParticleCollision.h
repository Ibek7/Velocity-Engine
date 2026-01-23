#pragma once

#include <vector>
#include <functional>

/**
 * @file ParticleCollision.h
 * @brief Particle collision detection and response system
 * 
 * Provides collision detection between particles and world geometry,
 * as well as particle-particle collisions with various response models.
 */

namespace Engine {

struct Vector3 {
    float x, y, z;
    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    Vector3 operator*(float s) const { return Vector3(x * s, y * s, z * s); }
    float dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    float length() const;
    Vector3 normalized() const;
};

/**
 * @struct Particle
 * @brief Particle data structure
 */
struct Particle {
    Vector3 position;
    Vector3 velocity;
    Vector3 force;
    float mass;
    float radius;
    float restitution;  ///< Bounciness (0-1)
    float friction;     ///< Surface friction coefficient
    int groupId;        ///< Collision group
    bool active;
    
    Particle() : mass(1.0f), radius(0.1f), restitution(0.5f), friction(0.3f), 
                groupId(0), active(true) {}
};

/**
 * @enum CollisionLayer
 * @brief Collision layer bit flags
 */
enum CollisionLayer : unsigned int {
    Layer_Default = 1 << 0,
    Layer_Debris = 1 << 1,
    Layer_Environment = 1 << 2,
    Layer_Effects = 1 << 3,
    Layer_All = 0xFFFFFFFF
};

/**
 * @struct CollisionPlane
 * @brief Infinite plane for collision
 */
struct CollisionPlane {
    Vector3 normal;
    float distance;
    float restitution;
    float friction;
    
    CollisionPlane() : distance(0.0f), restitution(0.8f), friction(0.5f) {}
};

/**
 * @struct CollisionSphere
 * @brief Sphere collider
 */
struct CollisionSphere {
    Vector3 center;
    float radius;
    float restitution;
    float friction;
    
    CollisionSphere() : radius(1.0f), restitution(0.6f), friction(0.4f) {}
};

/**
 * @struct CollisionBox
 * @brief Axis-aligned box collider
 */
struct CollisionBox {
    Vector3 min;
    Vector3 max;
    float restitution;
    float friction;
    
    CollisionBox() : restitution(0.5f), friction(0.6f) {}
};

/**
 * @class ParticleCollisionSystem
 * @brief Handles particle collision detection and response
 */
class ParticleCollisionSystem {
public:
    ParticleCollisionSystem();
    ~ParticleCollisionSystem();
    
    /**
     * @brief Add a collision plane
     * @param plane Plane to add
     * @return Plane ID
     */
    int addPlane(const CollisionPlane& plane);
    
    /**
     * @brief Add a collision sphere
     * @param sphere Sphere to add
     * @return Sphere ID
     */
    int addSphere(const CollisionSphere& sphere);
    
    /**
     * @brief Add a collision box
     * @param box Box to add
     * @return Box ID
     */
    int addBox(const CollisionBox& box);
    
    /**
     * @brief Remove a collider
     * @param type Collider type (0=plane, 1=sphere, 2=box)
     * @param id Collider ID
     */
    void removeCollider(int type, int id);
    
    /**
     * @brief Clear all colliders
     */
    void clearColliders();
    
    /**
     * @brief Detect and resolve collisions for particles
     * @param particles Particle array
     * @param count Number of particles
     * @param deltaTime Time step
     */
    void resolveCollisions(Particle* particles, int count, float deltaTime);
    
    /**
     * @brief Enable/disable particle-particle collisions
     * @param enabled Enable flag
     */
    void setParticleParticleCollisions(bool enabled) { m_particleCollisionsEnabled = enabled; }
    
    /**
     * @brief Check if particle-particle collisions are enabled
     * @return True if enabled
     */
    bool areParticleParticleCollisionsEnabled() const { return m_particleCollisionsEnabled; }
    
    /**
     * @brief Set collision layers for a particle group
     * @param groupId Group ID
     * @param layers Layer bit mask
     */
    void setCollisionLayers(int groupId, unsigned int layers);
    
    /**
     * @brief Check if two groups can collide
     * @param groupA First group ID
     * @param groupB Second group ID
     * @return True if collision enabled
     */
    bool canCollide(int groupA, int groupB) const;
    
    /**
     * @brief Set collision callback
     * @param callback Function called on collision
     */
    void setCollisionCallback(std::function<void(int, int, const Vector3&)> callback);
    
    /**
     * @brief Enable spatial partitioning for optimization
     * @param enabled Enable flag
     */
    void setSpatialPartitioning(bool enabled) { m_spatialPartitioningEnabled = enabled; }
    
    /**
     * @brief Set grid cell size for spatial partitioning
     * @param cellSize Cell size
     */
    void setGridCellSize(float cellSize) { m_gridCellSize = cellSize; }
    
    /**
     * @brief Get collision statistics
     * @param outTests Number of collision tests
     * @param outCollisions Number of actual collisions
     */
    void getStatistics(int& outTests, int& outCollisions) const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();

private:
    std::vector<CollisionPlane> m_planes;
    std::vector<CollisionSphere> m_spheres;
    std::vector<CollisionBox> m_boxes;
    
    bool m_particleCollisionsEnabled;
    bool m_spatialPartitioningEnabled;
    float m_gridCellSize;
    
    std::vector<unsigned int> m_collisionLayers;
    std::function<void(int, int, const Vector3&)> m_collisionCallback;
    
    int m_collisionTests;
    int m_collisionCount;
    
    bool checkPlaneCollision(Particle& particle, const CollisionPlane& plane, float deltaTime);
    bool checkSphereCollision(Particle& particle, const CollisionSphere& sphere, float deltaTime);
    bool checkBoxCollision(Particle& particle, const CollisionBox& box, float deltaTime);
    bool checkParticleCollision(Particle& p1, Particle& p2, float deltaTime);
    
    void resolveParticleCollision(Particle& p1, Particle& p2, const Vector3& normal);
    void buildSpatialGrid(Particle* particles, int count);
};

/**
 * @class ParticleConstraint
 * @brief Constraint between particles for cloth/soft-body simulation
 */
class ParticleConstraint {
public:
    /**
     * @brief Create distance constraint
     * @param p1 First particle index
     * @param p2 Second particle index
     * @param restLength Rest length
     * @param stiffness Constraint stiffness (0-1)
     */
    ParticleConstraint(int p1, int p2, float restLength, float stiffness = 1.0f);
    
    /**
     * @brief Solve constraint
     * @param particles Particle array
     */
    void solve(Particle* particles);
    
    /**
     * @brief Get first particle index
     */
    int getParticle1() const { return m_particle1; }
    
    /**
     * @brief Get second particle index
     */
    int getParticle2() const { return m_particle2; }
    
    /**
     * @brief Set stiffness
     * @param stiffness Stiffness value (0-1)
     */
    void setStiffness(float stiffness) { m_stiffness = stiffness; }

private:
    int m_particle1;
    int m_particle2;
    float m_restLength;
    float m_stiffness;
};

/**
 * @class ParticleForceField
 * @brief Applies forces to particles in a region
 */
class ParticleForceField {
public:
    enum class Type {
        Point,      ///< Point attractor/repulsor
        Directional, ///< Constant direction (like wind)
        Vortex,     ///< Spinning force
        Turbulence  ///< Random turbulence
    };
    
    ParticleForceField(Type type);
    
    /**
     * @brief Apply force to particle
     * @param particle Particle to affect
     * @param deltaTime Time step
     */
    void apply(Particle& particle, float deltaTime);
    
    /**
     * @brief Set force field position (for Point/Vortex)
     * @param position Position
     */
    void setPosition(const Vector3& position) { m_position = position; }
    
    /**
     * @brief Set force direction (for Directional)
     * @param direction Direction vector
     */
    void setDirection(const Vector3& direction) { m_direction = direction; }
    
    /**
     * @brief Set force strength
     * @param strength Force magnitude
     */
    void setStrength(float strength) { m_strength = strength; }
    
    /**
     * @brief Set influence radius
     * @param radius Radius of influence
     */
    void setRadius(float radius) { m_radius = radius; }
    
    /**
     * @brief Set falloff exponent
     * @param falloff Falloff exponent (1=linear, 2=quadratic)
     */
    void setFalloff(float falloff) { m_falloff = falloff; }

private:
    Type m_type;
    Vector3 m_position;
    Vector3 m_direction;
    float m_strength;
    float m_radius;
    float m_falloff;
    float m_noiseScale;
};

} // namespace Engine
