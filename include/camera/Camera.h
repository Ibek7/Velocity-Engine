#ifndef CAMERA_H
#define CAMERA_H

#include "math/Vector2D.h"
#include "math/Matrix3x3.h"
#include "ecs/Entity.h"
#include <memory>

namespace JJM {
namespace Graphics {

class Camera {
private:
    Math::Vector2D position;
    Math::Vector2D viewportSize;
    float zoom;
    float rotation;
    Math::Vector2D offset;
    
    // Follow target
    ECS::Entity* followTarget;
    float followSmoothing;
    Math::Vector2D followOffset;
    
    // Bounds
    bool hasBounds;
    Math::Vector2D boundsMin;
    Math::Vector2D boundsMax;
    
    // Shake effect
    bool isShaking;
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;
    Math::Vector2D shakeOffset;
    
public:
    Camera(const Math::Vector2D& viewSize);
    
    void update(float deltaTime);
    
    // Transform methods
    Math::Vector2D worldToScreen(const Math::Vector2D& worldPos) const;
    Math::Vector2D screenToWorld(const Math::Vector2D& screenPos) const;
    Math::Matrix3x3 getViewMatrix() const;
    
    // Position and viewport
    void setPosition(const Math::Vector2D& pos);
    void move(const Math::Vector2D& delta);
    const Math::Vector2D& getPosition() const { return position; }
    
    void setViewportSize(const Math::Vector2D& size) { viewportSize = size; }
    const Math::Vector2D& getViewportSize() const { return viewportSize; }
    
    // Zoom
    void setZoom(float z);
    void adjustZoom(float delta);
    float getZoom() const { return zoom; }
    
    // Rotation
    void setRotation(float rot);
    void rotate(float delta);
    float getRotation() const { return rotation; }
    
    // Offset (for screen shake, etc.)
    void setOffset(const Math::Vector2D& off) { offset = off; }
    const Math::Vector2D& getOffset() const { return offset; }
    
    // Follow target
    void setFollowTarget(ECS::Entity* target, float smoothing = 5.0f);
    void setFollowOffset(const Math::Vector2D& off) { followOffset = off; }
    void clearFollowTarget();
    
    // Bounds
    void setBounds(const Math::Vector2D& min, const Math::Vector2D& max);
    void clearBounds();
    
    // Camera shake
    void shake(float intensity, float duration);
    void stopShake();
    
    // Utility
    bool isInView(const Math::Vector2D& point, float margin = 0.0f) const;
    bool isRectInView(const Math::Vector2D& pos, const Math::Vector2D& size) const;
    Math::Vector2D getCenter() const;
};

} // namespace Graphics
} // namespace JJM

#endif // CAMERA_H
