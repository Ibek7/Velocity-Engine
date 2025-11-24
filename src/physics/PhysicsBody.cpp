#include "physics/PhysicsBody.h"
#include <cmath>

namespace JJM {
namespace Physics {

PhysicsBody::PhysicsBody()
    : position(0, 0), velocity(0, 0), acceleration(0, 0), force(0, 0),
      mass(1.0f), inverseMass(1.0f), restitution(0.5f), friction(0.3f),
      angularVelocity(0.0f), rotation(0.0f),
      type(BodyType::DYNAMIC), useGravity(true) {}

PhysicsBody::PhysicsBody(const Math::Vector2D& pos, float m)
    : position(pos), velocity(0, 0), acceleration(0, 0), force(0, 0),
      mass(m), restitution(0.5f), friction(0.3f),
      angularVelocity(0.0f), rotation(0.0f),
      type(BodyType::DYNAMIC), useGravity(true) {
    inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
}

void PhysicsBody::init() {
    // Initialization if needed
}

void PhysicsBody::update(float deltaTime) {
    if (type == BodyType::STATIC) {
        velocity = Math::Vector2D(0, 0);
        return;
    }
    
    // Calculate acceleration from forces
    if (inverseMass > 0.0f) {
        acceleration = force * inverseMass;
    }
    
    // Update velocity
    velocity += acceleration * deltaTime;
    
    // Update position
    position += velocity * deltaTime;
    
    // Update rotation
    rotation += angularVelocity * deltaTime;
    
    // Clear forces
    clearForces();
}

void PhysicsBody::applyForce(const Math::Vector2D& f) {
    if (type == BodyType::DYNAMIC) {
        force += f;
    }
}

void PhysicsBody::applyImpulse(const Math::Vector2D& impulse) {
    if (type == BodyType::DYNAMIC && inverseMass > 0.0f) {
        velocity += impulse * inverseMass;
    }
}

void PhysicsBody::clearForces() {
    force = Math::Vector2D(0, 0);
    acceleration = Math::Vector2D(0, 0);
}

void PhysicsBody::setMass(float m) {
    mass = m;
    inverseMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;
}

void PhysicsBody::setBodyType(BodyType t) {
    type = t;
    if (type == BodyType::STATIC) {
        velocity = Math::Vector2D(0, 0);
        inverseMass = 0.0f;
    }
}

} // namespace Physics
} // namespace JJM
