#include "physics/Collider.h"
#include "physics/PhysicsBody.h"
#include "ecs/Entity.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Physics {

// Collider base class
Collider::Collider(ColliderType t) 
    : type(t), offset(0, 0), isTrigger(false) {}

Math::Vector2D Collider::getPosition() const {
    if (owner) {
        auto* body = owner->getComponent<PhysicsBody>();
        if (body) {
            return body->position + offset;
        }
    }
    return offset;
}

// CircleCollider
CircleCollider::CircleCollider(float r) 
    : Collider(ColliderType::CIRCLE), radius(r) {}

bool CircleCollider::checkCollision(const Collider* other, CollisionInfo& info) const {
    if (!other) return false;
    
    switch (other->type) {
        case ColliderType::CIRCLE:
            return checkCircleCircle(static_cast<const CircleCollider*>(other), info);
        case ColliderType::BOX:
            return checkCircleBox(static_cast<const BoxCollider*>(other), info);
        default:
            return false;
    }
}

bool CircleCollider::checkCircleCircle(const CircleCollider* other, CollisionInfo& info) const {
    Math::Vector2D pos1 = getPosition();
    Math::Vector2D pos2 = other->getPosition();
    
    Math::Vector2D diff = pos2 - pos1;
    float distSquared = diff.magnitudeSquared();
    float radiusSum = radius + other->radius;
    
    if (distSquared < radiusSum * radiusSum) {
        float dist = std::sqrt(distSquared);
        info.colliding = true;
        info.penetration = radiusSum - dist;
        info.normal = (dist > 0.0f) ? diff / dist : Math::Vector2D(1, 0);
        info.other = other->owner;
        return true;
    }
    
    return false;
}

bool CircleCollider::checkCircleBox(const BoxCollider* other, CollisionInfo& info) const {
    Math::Vector2D circlePos = getPosition();
    Math::Vector2D boxMin = other->getMin();
    Math::Vector2D boxMax = other->getMax();
    
    // Find closest point on box to circle
    Math::Vector2D closest(
        std::max(boxMin.x, std::min(circlePos.x, boxMax.x)),
        std::max(boxMin.y, std::min(circlePos.y, boxMax.y))
    );
    
    Math::Vector2D diff = circlePos - closest;
    float distSquared = diff.magnitudeSquared();
    
    if (distSquared < radius * radius) {
        float dist = std::sqrt(distSquared);
        info.colliding = true;
        info.penetration = radius - dist;
        info.normal = (dist > 0.0f) ? diff / dist : Math::Vector2D(0, -1);
        info.other = other->owner;
        return true;
    }
    
    return false;
}

// BoxCollider
BoxCollider::BoxCollider(const Math::Vector2D& s) 
    : Collider(ColliderType::BOX), size(s) {}

BoxCollider::BoxCollider(float width, float height) 
    : Collider(ColliderType::BOX), size(width, height) {}

bool BoxCollider::checkCollision(const Collider* other, CollisionInfo& info) const {
    if (!other) return false;
    
    switch (other->type) {
        case ColliderType::CIRCLE:
            return checkBoxCircle(static_cast<const CircleCollider*>(other), info);
        case ColliderType::BOX:
            return checkBoxBox(static_cast<const BoxCollider*>(other), info);
        default:
            return false;
    }
}

Math::Vector2D BoxCollider::getMin() const {
    Math::Vector2D pos = getPosition();
    return pos - size * 0.5f;
}

Math::Vector2D BoxCollider::getMax() const {
    Math::Vector2D pos = getPosition();
    return pos + size * 0.5f;
}

bool BoxCollider::checkBoxBox(const BoxCollider* other, CollisionInfo& info) const {
    Math::Vector2D min1 = getMin();
    Math::Vector2D max1 = getMax();
    Math::Vector2D min2 = other->getMin();
    Math::Vector2D max2 = other->getMax();
    
    // AABB collision detection
    if (max1.x < min2.x || min1.x > max2.x ||
        max1.y < min2.y || min1.y > max2.y) {
        return false;
    }
    
    // Calculate overlap on each axis
    float overlapX = std::min(max1.x - min2.x, max2.x - min1.x);
    float overlapY = std::min(max1.y - min2.y, max2.y - min1.y);
    
    info.colliding = true;
    
    // Use smallest overlap as penetration
    if (overlapX < overlapY) {
        info.penetration = overlapX;
        info.normal = (getPosition().x < other->getPosition().x) 
            ? Math::Vector2D(-1, 0) : Math::Vector2D(1, 0);
    } else {
        info.penetration = overlapY;
        info.normal = (getPosition().y < other->getPosition().y) 
            ? Math::Vector2D(0, -1) : Math::Vector2D(0, 1);
    }
    
    info.other = other->owner;
    return true;
}

bool BoxCollider::checkBoxCircle(const CircleCollider* other, CollisionInfo& info) const {
    // Use circle-box collision (reversed)
    bool result = other->checkCircleBox(this, info);
    if (result) {
        info.normal = -info.normal;
    }
    return result;
}

} // namespace Physics
} // namespace JJM
