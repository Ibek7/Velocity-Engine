#include "camera/Camera.h"
#include <cmath>
#include <random>

namespace JJM {
namespace Graphics {

static std::random_device rd;
static std::mt19937 gen(rd());

Camera::Camera(const Math::Vector2D& viewSize)
    : position(0, 0), viewportSize(viewSize), zoom(1.0f), rotation(0),
      offset(0, 0), followTarget(nullptr), followSmoothing(5.0f),
      followOffset(0, 0), hasBounds(false), boundsMin(0, 0), boundsMax(0, 0),
      isShaking(false), shakeIntensity(0), shakeDuration(0), shakeTimer(0),
      shakeOffset(0, 0) {}

void Camera::update(float deltaTime) {
    // Update follow target
    if (followTarget) {
        Math::Vector2D targetPos = Math::Vector2D(0, 0); // Get from entity position
        Math::Vector2D desiredPos = targetPos + followOffset;
        
        if (followSmoothing > 0) {
            position = position.lerp(desiredPos, 1.0f - std::exp(-followSmoothing * deltaTime));
        } else {
            position = desiredPos;
        }
    }
    
    // Apply bounds
    if (hasBounds) {
        float halfWidth = (viewportSize.x / zoom) / 2.0f;
        float halfHeight = (viewportSize.y / zoom) / 2.0f;
        
        position.x = std::max(boundsMin.x + halfWidth, std::min(boundsMax.x - halfWidth, position.x));
        position.y = std::max(boundsMin.y + halfHeight, std::min(boundsMax.y - halfHeight, position.y));
    }
    
    // Update shake
    if (isShaking) {
        shakeTimer += deltaTime;
        
        if (shakeTimer >= shakeDuration) {
            stopShake();
        } else {
            std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
            float progress = shakeTimer / shakeDuration;
            float currentIntensity = shakeIntensity * (1.0f - progress);
            
            shakeOffset.x = dis(gen) * currentIntensity;
            shakeOffset.y = dis(gen) * currentIntensity;
        }
    }
}

Math::Vector2D Camera::worldToScreen(const Math::Vector2D& worldPos) const {
    Math::Vector2D effectivePos = position + offset + shakeOffset;
    Math::Vector2D relativePos = worldPos - effectivePos;
    
    // Apply zoom
    relativePos *= zoom;
    
    // Apply rotation
    if (std::abs(rotation) > 1e-6f) {
        relativePos = relativePos.rotate(rotation);
    }
    
    // Center on screen
    return relativePos + viewportSize * 0.5f;
}

Math::Vector2D Camera::screenToWorld(const Math::Vector2D& screenPos) const {
    Math::Vector2D effectivePos = position + offset + shakeOffset;
    
    // Uncenter from screen
    Math::Vector2D relativePos = screenPos - viewportSize * 0.5f;
    
    // Unapply rotation
    if (std::abs(rotation) > 1e-6f) {
        relativePos = relativePos.rotate(-rotation);
    }
    
    // Unapply zoom
    relativePos /= zoom;
    
    return relativePos + effectivePos;
}

Math::Matrix3x3 Camera::getViewMatrix() const {
    Math::Vector2D effectivePos = position + offset + shakeOffset;
    
    // Translation to center, then rotation, then zoom, then translation to viewport center
    Math::Matrix3x3 translate = Math::Matrix3x3::Translation(-effectivePos);
    Math::Matrix3x3 rotate = Math::Matrix3x3::Rotation(rotation);
    Math::Matrix3x3 scale = Math::Matrix3x3::Scale(zoom);
    Math::Matrix3x3 toViewport = Math::Matrix3x3::Translation(viewportSize * 0.5f);
    
    return toViewport * scale * rotate * translate;
}

void Camera::setPosition(const Math::Vector2D& pos) {
    position = pos;
}

void Camera::move(const Math::Vector2D& delta) {
    position += delta;
}

void Camera::setZoom(float z) {
    zoom = std::max(0.1f, std::min(10.0f, z));
}

void Camera::adjustZoom(float delta) {
    setZoom(zoom + delta);
}

void Camera::setRotation(float rot) {
    rotation = rot;
}

void Camera::rotate(float delta) {
    rotation += delta;
}

void Camera::setFollowTarget(ECS::Entity* target, float smoothing) {
    followTarget = target;
    followSmoothing = smoothing;
}

void Camera::clearFollowTarget() {
    followTarget = nullptr;
}

void Camera::setBounds(const Math::Vector2D& min, const Math::Vector2D& max) {
    hasBounds = true;
    boundsMin = min;
    boundsMax = max;
}

void Camera::clearBounds() {
    hasBounds = false;
}

void Camera::shake(float intensity, float duration) {
    isShaking = true;
    shakeIntensity = intensity;
    shakeDuration = duration;
    shakeTimer = 0;
}

void Camera::stopShake() {
    isShaking = false;
    shakeOffset = Math::Vector2D(0, 0);
}

bool Camera::isInView(const Math::Vector2D& point, float margin) const {
    Math::Vector2D screenPos = worldToScreen(point);
    return screenPos.x >= -margin && screenPos.x <= viewportSize.x + margin &&
           screenPos.y >= -margin && screenPos.y <= viewportSize.y + margin;
}

bool Camera::isRectInView(const Math::Vector2D& pos, const Math::Vector2D& size) const {
    Math::Vector2D topLeft = worldToScreen(pos);
    Math::Vector2D bottomRight = worldToScreen(pos + size);
    
    return !(bottomRight.x < 0 || topLeft.x > viewportSize.x ||
             bottomRight.y < 0 || topLeft.y > viewportSize.y);
}

Math::Vector2D Camera::getCenter() const {
    return position;
}

Camera::ViewportInfo Camera::getViewportBounds() const {
    ViewportInfo info;
    float halfWidth = (viewportSize.x / zoom) * 0.5f;
    float halfHeight = (viewportSize.y / zoom) * 0.5f;
    
    Math::Vector2D center = position + offset + shakeOffset;
    info.topLeft = Math::Vector2D(center.x - halfWidth, center.y - halfHeight);
    info.bottomRight = Math::Vector2D(center.x + halfWidth, center.y + halfHeight);
    info.width = halfWidth * 2.0f;
    info.height = halfHeight * 2.0f;
    
    return info;
}

bool Camera::shouldCull(const Math::Vector2D& position, float radius) const {
    ViewportInfo vp = getViewportBounds();
    
    // Check if circle intersects viewport
    if (position.x + radius < vp.topLeft.x) return true;
    if (position.x - radius > vp.bottomRight.x) return true;
    if (position.y + radius < vp.topLeft.y) return true;
    if (position.y - radius > vp.bottomRight.y) return true;
    
    return false;
}

bool Camera::shouldCullRect(const Math::Vector2D& min, const Math::Vector2D& max) const {
    ViewportInfo vp = getViewportBounds();
    
    // Check if rectangles overlap
    if (max.x < vp.topLeft.x) return true;
    if (min.x > vp.bottomRight.x) return true;
    if (max.y < vp.topLeft.y) return true;
    if (min.y > vp.bottomRight.y) return true;
    
    return false;
}

float Camera::getVisibilityScore(const Math::Vector2D& position) const {
    ViewportInfo vp = getViewportBounds();
    Math::Vector2D center = (vp.topLeft + vp.bottomRight) * 0.5f;
    
    // Distance from viewport center
    float dx = std::abs(position.x - center.x) / (vp.width * 0.5f);
    float dy = std::abs(position.y - center.y) / (vp.height * 0.5f);
    float distance = std::max(dx, dy);
    
    // Score: 1.0 at center, 0.0 at edge, negative outside
    return 1.0f - distance;
}

float Camera::getRectVisibilityScore(const Math::Vector2D& min, const Math::Vector2D& max) const {
    ViewportInfo vp = getViewportBounds();
    
    // Calculate overlap area
    float overlapMinX = std::max(min.x, vp.topLeft.x);
    float overlapMaxX = std::min(max.x, vp.bottomRight.x);
    float overlapMinY = std::max(min.y, vp.topLeft.y);
    float overlapMaxY = std::min(max.y, vp.bottomRight.y);
    
    if (overlapMinX >= overlapMaxX || overlapMinY >= overlapMaxY) {
        return 0.0f; // No overlap
    }
    
    float overlapArea = (overlapMaxX - overlapMinX) * (overlapMaxY - overlapMinY);
    float rectArea = (max.x - min.x) * (max.y - min.y);
    
    // Score: ratio of visible area to total area
    return overlapArea / rectArea;
}

void Camera::getFrustumCorners(Math::Vector2D corners[4]) const {
    ViewportInfo vp = getViewportBounds();
    
    corners[0] = vp.topLeft;
    corners[1] = Math::Vector2D(vp.bottomRight.x, vp.topLeft.y);
    corners[2] = vp.bottomRight;
    corners[3] = Math::Vector2D(vp.topLeft.x, vp.bottomRight.y);
}

} // namespace Graphics
} // namespace JJM
