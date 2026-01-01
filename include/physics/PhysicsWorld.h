#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include "math/Vector2D.h"
#include "physics/PhysicsBody.h"
#include "physics/Collider.h"
#include "ecs/EntityManager.h"
#include <vector>
#include <functional>
#include <optional>

namespace JJM {
namespace Physics {

// Raycast hit result
struct RaycastHit {
    ECS::Entity* entity;
    Math::Vector2D point;
    Math::Vector2D normal;
    float distance;
    Collider* collider;
};

// Layer mask for collision filtering
using LayerMask = uint32_t;

// Collision filter callback
using CollisionFilter = std::function<bool(ECS::Entity*, ECS::Entity*)>;

// Physics configuration
struct PhysicsConfig {
    Math::Vector2D gravity;
    float fixedTimeStep;
    int velocityIterations;
    int positionIterations;
    bool continuousCollision;
    float sleepThreshold;
    
    PhysicsConfig()
        : gravity(0.0f, -9.81f)
        , fixedTimeStep(1.0f / 60.0f)
        , velocityIterations(8)
        , positionIterations(3)
        , continuousCollision(true)
        , sleepThreshold(0.01f)
    {}
};

class PhysicsWorld {
private:
    Math::Vector2D gravity;
    ECS::EntityManager* entityManager;
    std::vector<CollisionInfo> collisions;
    PhysicsConfig config;
    
    // Layer collision matrix
    std::vector<std::vector<bool>> layerMatrix;
    static constexpr int MAX_LAYERS = 32;
    
    // Collision callbacks
    std::function<void(const CollisionInfo&)> onCollisionEnter;
    std::function<void(const CollisionInfo&)> onCollisionStay;
    std::function<void(const CollisionInfo&)> onCollisionExit;
    
    // Time accumulator for fixed timestep
    float timeAccumulator;

public:
    PhysicsWorld(ECS::EntityManager* em);
    ~PhysicsWorld();

    // Configuration
    void setConfig(const PhysicsConfig& cfg);
    const PhysicsConfig& getConfig() const { return config; }

    // Update physics
    void update(float deltaTime);
    void fixedUpdate();
    
    // Gravity
    void setGravity(const Math::Vector2D& g) { gravity = g; config.gravity = g; }
    Math::Vector2D getGravity() const { return gravity; }
    
    // Collision detection
    void detectCollisions();
    void resolveCollisions();
    
    // Raycasting
    std::optional<RaycastHit> raycast(const Math::Vector2D& origin, 
                                       const Math::Vector2D& direction,
                                       float maxDistance,
                                       LayerMask layerMask = 0xFFFFFFFF) const;
    std::vector<RaycastHit> raycastAll(const Math::Vector2D& origin,
                                        const Math::Vector2D& direction,
                                        float maxDistance,
                                        LayerMask layerMask = 0xFFFFFFFF) const;
    bool linecast(const Math::Vector2D& start, 
                  const Math::Vector2D& end,
                  LayerMask layerMask = 0xFFFFFFFF) const;
    
    // Shape queries
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point);
    std::vector<ECS::Entity*> queryArea(const Math::Vector2D& min, const Math::Vector2D& max);
    std::vector<ECS::Entity*> queryCircle(const Math::Vector2D& center, float radius);
    std::vector<ECS::Entity*> overlapBox(const Math::Vector2D& center, 
                                          const Math::Vector2D& halfExtents,
                                          float angle = 0.0f);
    
    // Layer management
    void setLayerCollision(int layer1, int layer2, bool shouldCollide);
    bool getLayerCollision(int layer1, int layer2) const;
    void resetLayerMatrix();
    
    // Collision callbacks
    void setOnCollisionEnter(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionStay(std::function<void(const CollisionInfo&)> callback);
    void setOnCollisionExit(std::function<void(const CollisionInfo&)> callback);
    
    // Physics queries
    Math::Vector2D getClosestPoint(const Math::Vector2D& point, const Collider& collider) const;
    float getDistance(ECS::Entity* entityA, ECS::Entity* entityB) const;
    bool checkCollision(ECS::Entity* entityA, ECS::Entity* entityB) const;
    
    // Force application
    void applyExplosionForce(const Math::Vector2D& center, float force, float radius);
    void applyForceInArea(const Math::Vector2D& min, const Math::Vector2D& max, 
                          const Math::Vector2D& force);
    
private:
    void applyGravity(float deltaTime);
    void resolveCollision(const CollisionInfo& info);
    bool rayIntersectsCollider(const Math::Vector2D& origin,
                               const Math::Vector2D& direction,
                               const Collider& collider,
                               RaycastHit& outHit) const;
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_WORLD_H
