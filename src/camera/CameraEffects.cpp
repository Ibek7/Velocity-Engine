#include "camera/CameraEffects.h"
#include "camera/Camera.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace JJM {
namespace Camera {

static float randomFloat(float min, float max) {
    return min + (max - min) * (static_cast<float>(rand()) / RAND_MAX);
}

// CameraShake implementation
CameraShake::CameraShake()
    : active(false), intensity(0), duration(0), elapsed(0),
      frequency(10.0f), offset(0, 0), shakeMode(ShakeMode::Random),
      noiseX(0), noiseY(0) {}

CameraShake::~CameraShake() {}

void CameraShake::start(float intensity, float duration, float frequency) {
    this->intensity = intensity;
    this->duration = duration;
    this->frequency = frequency;
    this->elapsed = 0;
    this->active = true;
    this->noiseX = randomFloat(0, 1000);
    this->noiseY = randomFloat(0, 1000);
}

void CameraShake::stop() {
    active = false;
    offset = Math::Vector2D(0, 0);
}

void CameraShake::update(float deltaTime) {
    if (!active) return;
    
    elapsed += deltaTime;
    
    if (elapsed >= duration) {
        stop();
        return;
    }
    
    float t = elapsed / duration;
    float currentIntensity = intensity * (1.0f - t);
    
    switch (shakeMode) {
        case ShakeMode::Random:
            offset = calculateRandomShake() * currentIntensity;
            break;
        case ShakeMode::Sine:
            offset = calculateSineShake() * currentIntensity;
            break;
        case ShakeMode::Perlin:
            offset = calculatePerlinShake() * currentIntensity;
            break;
    }
}

Math::Vector2D CameraShake::calculateRandomShake() {
    return Math::Vector2D(
        randomFloat(-1.0f, 1.0f),
        randomFloat(-1.0f, 1.0f)
    );
}

Math::Vector2D CameraShake::calculateSineShake() {
    float angle = elapsed * frequency * 2.0f * M_PI;
    return Math::Vector2D(
        std::sin(angle),
        std::cos(angle)
    );
}

Math::Vector2D CameraShake::calculatePerlinShake() {
    noiseX += frequency * 0.01f;
    noiseY += frequency * 0.01f;
    
    return Math::Vector2D(
        std::sin(noiseX) * std::cos(noiseY * 0.7f),
        std::cos(noiseX * 0.7f) * std::sin(noiseY)
    );
}

// CameraZoom implementation
CameraZoom::CameraZoom()
    : currentZoom(1.0f), targetZoom(1.0f), zoomSpeed(2.0f),
      minZoom(0.5f), maxZoom(3.0f), zoomDuration(0.5f),
      zoomElapsed(0), zooming(false) {}

CameraZoom::~CameraZoom() {}

void CameraZoom::setTargetZoom(float target, float duration) {
    targetZoom = std::clamp(target, minZoom, maxZoom);
    zoomDuration = duration;
    zoomElapsed = 0;
    zooming = true;
}

void CameraZoom::setZoom(float zoom) {
    currentZoom = std::clamp(zoom, minZoom, maxZoom);
    targetZoom = currentZoom;
    zooming = false;
}

void CameraZoom::update(float deltaTime) {
    if (!zooming) return;
    
    zoomElapsed += deltaTime;
    float t = std::min(zoomElapsed / zoomDuration, 1.0f);
    
    currentZoom = currentZoom + (targetZoom - currentZoom) * t;
    
    if (t >= 1.0f) {
        currentZoom = targetZoom;
        zooming = false;
    }
}

void CameraZoom::setZoomLimits(float min, float max) {
    minZoom = min;
    maxZoom = max;
    currentZoom = std::clamp(currentZoom, minZoom, maxZoom);
    targetZoom = std::clamp(targetZoom, minZoom, maxZoom);
}

// CameraFollow implementation
CameraFollow::CameraFollow()
    : targetPosition(0, 0), offset(0, 0), followSpeed(5.0f),
      deadzoneWidth(0), deadzoneHeight(0),
      lookAheadEnabled(false), lookAheadDistance(50.0f),
      predictionEnabled(false), predictionFactor(0.5f),
      previousTargetPosition(0, 0) {}

CameraFollow::~CameraFollow() {}

void CameraFollow::setTarget(const Math::Vector2D& target) {
    previousTargetPosition = targetPosition;
    targetPosition = target;
}

void CameraFollow::setDeadzone(float width, float height) {
    deadzoneWidth = width;
    deadzoneHeight = height;
}

Math::Vector2D CameraFollow::update(const Math::Vector2D& currentPosition, float deltaTime) {
    Math::Vector2D effectiveTarget = targetPosition + offset;
    
    if (predictionEnabled) {
        Math::Vector2D velocity = (targetPosition - previousTargetPosition) / deltaTime;
        effectiveTarget = effectiveTarget + velocity * predictionFactor;
    }
    
    if (lookAheadEnabled) {
        Math::Vector2D direction = targetPosition - previousTargetPosition;
        direction.normalize();
        effectiveTarget = effectiveTarget + direction * lookAheadDistance;
    }
    
    Math::Vector2D diff = effectiveTarget - currentPosition;
    
    if (deadzoneWidth > 0 && deadzoneHeight > 0) {
        if (std::abs(diff.x) < deadzoneWidth * 0.5f) diff.x = 0;
        if (std::abs(diff.y) < deadzoneHeight * 0.5f) diff.y = 0;
    }
    
    return currentPosition + diff * followSpeed * deltaTime;
}

void CameraFollow::setLookAhead(bool enabled, float distance) {
    lookAheadEnabled = enabled;
    lookAheadDistance = distance;
}

void CameraFollow::setPrediction(bool enabled, float factor) {
    predictionEnabled = enabled;
    predictionFactor = factor;
}

// CameraEffects implementation
CameraEffects::CameraEffects(Graphics::Camera* camera)
    : camera(camera), basePosition(0, 0) {}

CameraEffects::~CameraEffects() {}

void CameraEffects::update(float deltaTime) {
    cameraShake.update(deltaTime);
    zoom.update(deltaTime);
}

void CameraEffects::shake(float intensity, float duration, float frequency) {
    cameraShake.start(intensity, duration, frequency);
}

void CameraEffects::zoomTo(float target, float duration) {
    zoom.setTargetZoom(target, duration);
}

void CameraEffects::followTarget(const Math::Vector2D& target) {
    follow.setTarget(target);
}

void CameraEffects::applyEffects() {
    if (!camera) return;
    
    Math::Vector2D position = camera->getPosition();
    
    position = follow.update(position, 0.016f);
    position = position + cameraShake.getOffset();
    
    camera->setPosition(position);
}

// CameraTransition implementation
CameraTransition::CameraTransition()
    : active(false), startPosition(0, 0), targetPosition(0, 0),
      duration(0), elapsed(0), easingFunction(nullptr) {}

CameraTransition::~CameraTransition() {}

void CameraTransition::transitionTo(const Math::Vector2D& target, float duration,
                                   std::function<float(float)> easingFunc) {
    targetPosition = target;
    this->duration = duration;
    elapsed = 0;
    active = true;
    easingFunction = easingFunc ? easingFunc : easeInOut;
}

void CameraTransition::update(float deltaTime) {
    if (!active) return;
    
    elapsed += deltaTime;
    float t = std::min(elapsed / duration, 1.0f);
    
    if (easingFunction) {
        t = easingFunction(t);
    }
    
    if (t >= 1.0f) {
        active = false;
    }
}

Math::Vector2D CameraTransition::getCurrentPosition() const {
    if (!active) return targetPosition;
    
    float t = std::min(elapsed / duration, 1.0f);
    if (easingFunction) {
        t = easingFunction(t);
    }
    
    return startPosition + (targetPosition - startPosition) * t;
}

void CameraTransition::cancel() {
    active = false;
}

float CameraTransition::linearEasing(float t) {
    return t;
}

float CameraTransition::easeInOut(float t) {
    return t < 0.5f ? 2 * t * t : -1 + (4 - 2 * t) * t;
}

// CameraBounds implementation
CameraBounds::CameraBounds()
    : enabled(false), minX(0), minY(0), maxX(0), maxY(0) {}

CameraBounds::~CameraBounds() {}

void CameraBounds::setBounds(float minX, float minY, float maxX, float maxY) {
    this->minX = minX;
    this->minY = minY;
    this->maxX = maxX;
    this->maxY = maxY;
    this->enabled = true;
}

Math::Vector2D CameraBounds::clampPosition(const Math::Vector2D& position,
                                          float cameraWidth, float cameraHeight) const {
    if (!enabled) return position;
    
    float halfWidth = cameraWidth * 0.5f;
    float halfHeight = cameraHeight * 0.5f;
    
    return Math::Vector2D(
        std::clamp(position.x, minX + halfWidth, maxX - halfWidth),
        std::clamp(position.y, minY + halfHeight, maxY - halfHeight)
    );
}

// AdvancedCamera implementation
AdvancedCamera::AdvancedCamera(Graphics::Camera* camera)
    : camera(camera), effects(camera), transition(), bounds() {}

AdvancedCamera::~AdvancedCamera() {}

void AdvancedCamera::update(float deltaTime) {
    effects.update(deltaTime);
    transition.update(deltaTime);
    
    if (transition.isTransitioning()) {
        camera->setPosition(transition.getCurrentPosition());
    }
    
    effects.applyEffects();
    
    Math::Vector2D position = camera->getPosition();
    position = bounds.clampPosition(position, 1920, 1080);
    camera->setPosition(position);
}

void AdvancedCamera::shake(float intensity, float duration, float frequency) {
    effects.shake(intensity, duration, frequency);
}

void AdvancedCamera::zoomTo(float target, float duration) {
    effects.zoomTo(target, duration);
}

void AdvancedCamera::followTarget(const Math::Vector2D& target) {
    effects.followTarget(target);
}

void AdvancedCamera::transitionTo(const Math::Vector2D& target, float duration) {
    transition.transitionTo(target, duration);
}

void AdvancedCamera::setBounds(float minX, float minY, float maxX, float maxY) {
    bounds.setBounds(minX, minY, maxX, maxY);
}

void AdvancedCamera::setDeadzone(float width, float height) {
    effects.getFollow().setDeadzone(width, height);
}

} // namespace Camera
} // namespace JJM
