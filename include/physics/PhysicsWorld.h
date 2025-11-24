#ifndef PHYSICS_WORLD_H
#define PHYSICS_WORLD_H

#include "math/Vector2D.h"
#include "physics/PhysicsBody.h"
#include "physics/Collider.h"
#include "ecs/EntityManager.h"
#include <vector>

namespace JJM {
namespace Physics {

class PhysicsWorld {
private:
    Math::Vector2D gravity;
    ECS::EntityManager* entityManager;
    std::vector<CollisionInfo> collisions;

public:
    PhysicsWorld(ECS::EntityManager* em);
    ~PhysicsWorld();

    // Update physics
    void update(float deltaTime);
    
    // Gravity
    void setGravity(const Math::Vector2D& g) { gravity = g; }
    Math::Vector2D getGravity() const { return gravity; }
    
    // Collision detection
    void detectCollisions();
    void resolveCollisions();
    
    // Query
    std::vector<ECS::Entity*> queryPoint(const Math::Vector2D& point);
    std::vector<ECS::Entity*> queryArea(const Math::Vector2D& min, const Math::Vector2D& max);
    
private:
    void applyGravity(float deltaTime);
    void resolveCollision(const CollisionInfo& info);
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_WORLD_H
