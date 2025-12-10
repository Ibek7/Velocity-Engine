#pragma once

#include "math/Vector2D.h"
#include <functional>
#include <vector>

namespace JJM {

namespace Graphics {
    class Camera; // Forward declaration
}

namespace Camera {

enum class ShakeMode {
    Random,
    Sine,
    Perlin
};

class CameraShake {
public:
    CameraShake();
    ~CameraShake();
    
    void start(float intensity, float duration, float frequency = 10.0f);
    void stop();
    
    void update(float deltaTime);
    Math::Vector2D getOffset() const { return offset; }
    
    void setMode(ShakeMode mode) { shakeMode = mode; }
    ShakeMode getMode() const { return shakeMode; }
    
    bool isShaking() const { return active; }

private:
    bool active;
    float intensity;
    float duration;
    float elapsed;
    float frequency;
    Math::Vector2D offset;
    ShakeMode shakeMode;
    
    float noiseX;
    float noiseY;
    
    Math::Vector2D calculateRandomShake();
    Math::Vector2D calculateSineShake();
    Math::Vector2D calculatePerlinShake();
};

class CameraZoom {
public:
    CameraZoom();
    ~CameraZoom();
    
    void setTargetZoom(float target, float duration = 0.5f);
    void setZoom(float zoom);
    
    void update(float deltaTime);
    
    float getCurrentZoom() const { return currentZoom; }
    float getTargetZoom() const { return targetZoom; }
    
    void setZoomLimits(float min, float max);
    void setZoomSpeed(float speed) { zoomSpeed = speed; }

private:
    float currentZoom;
    float targetZoom;
    float zoomSpeed;
    float minZoom;
    float maxZoom;
    float zoomDuration;
    float zoomElapsed;
    bool zooming;
};

class CameraFollow {
public:
    CameraFollow();
    ~CameraFollow();
    
    void setTarget(const Math::Vector2D& target);
    Math::Vector2D getTarget() const { return targetPosition; }
    
    void setFollowSpeed(float speed) { followSpeed = speed; }
    float getFollowSpeed() const { return followSpeed; }
    
    void setDeadzone(float width, float height);
    void setOffset(const Math::Vector2D& offset) { this->offset = offset; }
    
    Math::Vector2D update(const Math::Vector2D& currentPosition, float deltaTime);
    
    void setLookAhead(bool enabled, float distance = 50.0f);
    void setPrediction(bool enabled, float factor = 0.5f);

private:
    Math::Vector2D targetPosition;
    Math::Vector2D offset;
    float followSpeed;
    
    float deadzoneWidth;
    float deadzoneHeight;
    
    bool lookAheadEnabled;
    float lookAheadDistance;
    
    bool predictionEnabled;
    float predictionFactor;
    Math::Vector2D previousTargetPosition;
};

class CameraEffects {
public:
    CameraEffects(Graphics::Camera* camera);
    ~CameraEffects();
    
    void update(float deltaTime);
    
    CameraShake& getShake() { return cameraShake; }
    CameraZoom& getZoom() { return zoom; }
    CameraFollow& getFollow() { return follow; }
    
    void shake(float intensity, float duration, float frequency = 10.0f);
    void zoomTo(float target, float duration = 0.5f);
    void followTarget(const Math::Vector2D& target);
    
    void applyEffects();

private:
    Graphics::Camera* camera;
    CameraShake cameraShake;
    CameraZoom zoom;
    CameraFollow follow;
    
    Math::Vector2D basePosition;
};

class CameraTransition {
public:
    CameraTransition();
    ~CameraTransition();
    
    void transitionTo(const Math::Vector2D& target, float duration,
                     std::function<float(float)> easingFunc = nullptr);
    
    void update(float deltaTime);
    Math::Vector2D getCurrentPosition() const;
    
    bool isTransitioning() const { return active; }
    
    void cancel();

private:
    bool active;
    Math::Vector2D startPosition;
    Math::Vector2D targetPosition;
    float duration;
    float elapsed;
    std::function<float(float)> easingFunction;
    
    static float linearEasing(float t);
    static float easeInOut(float t);
};

class CameraBounds {
public:
    CameraBounds();
    ~CameraBounds();
    
    void setBounds(float minX, float minY, float maxX, float maxY);
    void setEnabled(bool enabled) { this->enabled = enabled; }
    
    Math::Vector2D clampPosition(const Math::Vector2D& position,
                                 float cameraWidth, float cameraHeight) const;
    
    bool isEnabled() const { return enabled; }

private:
    bool enabled;
    float minX, minY;
    float maxX, maxY;
};

class AdvancedCamera {
public:
    AdvancedCamera(Graphics::Camera* camera);
    ~AdvancedCamera();
    
    void update(float deltaTime);
    
    void shake(float intensity, float duration, float frequency = 10.0f);
    void zoomTo(float target, float duration = 0.5f);
    void followTarget(const Math::Vector2D& target);
    void transitionTo(const Math::Vector2D& target, float duration);
    
    void setBounds(float minX, float minY, float maxX, float maxY);
    void setDeadzone(float width, float height);
    
    CameraShake& getShake() { return effects.getShake(); }
    CameraZoom& getZoom() { return effects.getZoom(); }
    CameraFollow& getFollow() { return effects.getFollow(); }
    CameraBounds& getBounds() { return bounds; }

private:
    Graphics::Camera* camera;
    CameraEffects effects;
    CameraTransition transition;
    CameraBounds bounds;
};

} // namespace Camera
} // namespace JJM
