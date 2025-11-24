#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include "math/Vector2D.h"
#include "ecs/Component.h"

namespace JJM {
namespace Physics {

enum class BodyType {
    STATIC,
    KINEMATIC,
    DYNAMIC
};

class PhysicsBody : public ECS::Component {
public:
    Math::Vector2D position;
    Math::Vector2D velocity;
    Math::Vector2D acceleration;
    Math::Vector2D force;
    
    float mass;
    float inverseMass;
    float restitution;  // Bounciness (0-1)
    float friction;
    float angularVelocity;
    float rotation;
    
    BodyType type;
    bool useGravity;
    
    PhysicsBody();
    PhysicsBody(const Math::Vector2D& pos, float mass = 1.0f);
    
    void init() override;
    void update(float deltaTime) override;
    
    // Force application
    void applyForce(const Math::Vector2D& f);
    void applyImpulse(const Math::Vector2D& impulse);
    void clearForces();
    
    // Setters
    void setMass(float m);
    void setBodyType(BodyType t);
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setVelocity(const Math::Vector2D& vel) { velocity = vel; }
    
    // Getters
    Math::Vector2D getPosition() const { return position; }
    Math::Vector2D getVelocity() const { return velocity; }
    float getMass() const { return mass; }
    float getInverseMass() const { return inverseMass; }
};

} // namespace Physics
} // namespace JJM

#endif // PHYSICS_BODY_H
