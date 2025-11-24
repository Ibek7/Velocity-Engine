#include "physics/PhysicsWorld.h"
#include <algorithm>

namespace JJM {
namespace Physics {

PhysicsWorld::PhysicsWorld(ECS::EntityManager* em) 
    : gravity(0, 980.0f), entityManager(em) {}

PhysicsWorld::~PhysicsWorld() {}

void PhysicsWorld::update(float deltaTime) {
    if (!entityManager) return;
    
    // Apply gravity
    applyGravity(deltaTime);
    
    // Update physics bodies
    auto entities = entityManager->getEntitiesWithComponent<PhysicsBody>();
    for (auto* entity : entities) {
        auto* body = entity->getComponent<PhysicsBody>();
        if (body && body->isEnabled()) {
            body->update(deltaTime);
        }
    }
    
    // Detect and resolve collisions
    detectCollisions();
    resolveCollisions();
}

void PhysicsWorld::applyGravity(float deltaTime) {
    if (!entityManager) return;
    
    auto entities = entityManager->getEntitiesWithComponent<PhysicsBody>();
    for (auto* entity : entities) {
        auto* body = entity->getComponent<PhysicsBody>();
        if (body && body->isEnabled() && body->useGravity && body->type == BodyType::DYNAMIC) {
            body->applyForce(gravity * body->mass);
        }
    }
}

void PhysicsWorld::detectCollisions() {
    collisions.clear();
    
    if (!entityManager) return;
    
    auto entities = entityManager->getEntitiesWithComponent<Collider>();
    
    for (size_t i = 0; i < entities.size(); ++i) {
        auto* collider1 = entities[i]->getComponent<Collider>();
        if (!collider1 || !collider1->isEnabled()) continue;
        
        for (size_t j = i + 1; j < entities.size(); ++j) {
            auto* collider2 = entities[j]->getComponent<Collider>();
            if (!collider2 || !collider2->isEnabled()) continue;
            
            CollisionInfo info;
            if (collider1->checkCollision(collider2, info)) {
                collisions.push_back(info);
            }
        }
    }
}

void PhysicsWorld::resolveCollisions() {
    for (const auto& info : collisions) {
        if (!info.colliding) continue;
        resolveCollision(info);
    }
}

void PhysicsWorld::resolveCollision(const CollisionInfo& info) {
    if (!info.other) return;
    
    auto* body1 = info.other->getOwner()->getComponent<PhysicsBody>();
    auto* body2 = info.other->getComponent<PhysicsBody>();
    
    if (!body1 || !body2) return;
    
    // Skip if both are static
    if (body1->type == BodyType::STATIC && body2->type == BodyType::STATIC) {
        return;
    }
    
    // Calculate relative velocity
    Math::Vector2D relativeVel = body2->velocity - body1->velocity;
    float velAlongNormal = relativeVel.dot(info.normal);
    
    // Don't resolve if velocities are separating
    if (velAlongNormal > 0) return;
    
    // Calculate restitution (bounciness)
    float e = std::min(body1->restitution, body2->restitution);
    
    // Calculate impulse scalar
    float j = -(1.0f + e) * velAlongNormal;
    j /= body1->inverseMass + body2->inverseMass;
    
    // Apply impulse
    Math::Vector2D impulse = info.normal * j;
    
    if (body1->type == BodyType::DYNAMIC) {
        body1->velocity -= impulse * body1->inverseMass;
    }
    if (body2->type == BodyType::DYNAMIC) {
        body2->velocity += impulse * body2->inverseMass;
    }
    
    // Position correction to prevent sinking
    const float percent = 0.2f; // Penetration percentage to correct
    const float slop = 0.01f; // Penetration allowance
    float correctionMag = std::max(info.penetration - slop, 0.0f) / (body1->inverseMass + body2->inverseMass) * percent;
    Math::Vector2D correction = info.normal * correctionMag;
    
    if (body1->type == BodyType::DYNAMIC) {
        body1->position -= correction * body1->inverseMass;
    }
    if (body2->type == BodyType::DYNAMIC) {
        body2->position += correction * body2->inverseMass;
    }
}

std::vector<ECS::Entity*> PhysicsWorld::queryPoint(const Math::Vector2D& point) {
    std::vector<ECS::Entity*> result;
    
    if (!entityManager) return result;
    
    auto entities = entityManager->getEntitiesWithComponent<Collider>();
    
    for (auto* entity : entities) {
        auto* collider = entity->getComponent<Collider>();
        if (!collider || !collider->isEnabled()) continue;
        
        if (collider->type == ColliderType::CIRCLE) {
            auto* circle = static_cast<CircleCollider*>(collider);
            float dist = (point - circle->getPosition()).magnitude();
            if (dist <= circle->radius) {
                result.push_back(entity);
            }
        } else if (collider->type == ColliderType::BOX) {
            auto* box = static_cast<BoxCollider*>(collider);
            Math::Vector2D min = box->getMin();
            Math::Vector2D max = box->getMax();
            if (point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y) {
                result.push_back(entity);
            }
        }
    }
    
    return result;
}

std::vector<ECS::Entity*> PhysicsWorld::queryArea(const Math::Vector2D& min, const Math::Vector2D& max) {
    std::vector<ECS::Entity*> result;
    
    if (!entityManager) return result;
    
    auto entities = entityManager->getEntitiesWithComponent<Collider>();
    
    for (auto* entity : entities) {
        auto* collider = entity->getComponent<Collider>();
        if (!collider || !collider->isEnabled()) continue;
        
        if (collider->type == ColliderType::BOX) {
            auto* box = static_cast<BoxCollider*>(collider);
            Math::Vector2D boxMin = box->getMin();
            Math::Vector2D boxMax = box->getMax();
            
            if (!(boxMax.x < min.x || boxMin.x > max.x ||
                  boxMax.y < min.y || boxMin.y > max.y)) {
                result.push_back(entity);
            }
        }
    }
    
    return result;
}

} // namespace Physics
} // namespace JJM
