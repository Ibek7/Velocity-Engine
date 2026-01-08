#ifndef CAMERA_H
#define CAMERA_H

#include "math/Vector2D.h"
#include "math/Matrix3x3.h"
#include "ecs/Entity.h"
#include <memory>
#include <functional>
#include <vector>

namespace JJM {
namespace Graphics {

/**
 * @brief Camera follow modes
 */
enum class CameraFollowMode {
    Instant,            // Snap to target instantly
    Linear,             // Linear interpolation
    SmoothDamp,         // Smooth damping (critically damped spring)
    Spring,             // Springy follow with overshoot
    Predictive,         // Predict target movement
    DeadZone,           // Only follow outside dead zone
    LookAhead           // Look ahead based on velocity
};

/**
 * @brief Camera follow configuration
 */
struct CameraFollowConfig {
    CameraFollowMode mode;
    float smoothTime;           // Time to reach target (SmoothDamp)
    float springStiffness;      // Spring constant (Spring mode)
    float springDamping;        // Damping coefficient (Spring mode)
    float predictionTime;       // How far ahead to predict (Predictive)
    Math::Vector2D deadZoneSize;// Size of dead zone (DeadZone mode)
    float lookAheadDistance;    // Look ahead distance (LookAhead)
    float lookAheadSmoothing;   // Smoothing for look ahead
    float maxSpeed;             // Maximum camera speed
    
    CameraFollowConfig()
        : mode(CameraFollowMode::SmoothDamp)
        , smoothTime(0.3f)
        , springStiffness(20.0f)
        , springDamping(5.0f)
        , predictionTime(0.1f)
        , deadZoneSize(50.0f, 50.0f)
        , lookAheadDistance(100.0f)
        , lookAheadSmoothing(5.0f)
        , maxSpeed(1000.0f)
    {}
};

/**
 * @brief Camera constraint types
 */
struct CameraConstraints {
    bool enableMinZoom;
    bool enableMaxZoom;
    float minZoom;
    float maxZoom;
    
    bool enableBounds;
    Math::Vector2D boundsMin;
    Math::Vector2D boundsMax;
    
    bool enableRotationLimits;
    float minRotation;
    float maxRotation;
    
    CameraConstraints()
        : enableMinZoom(false)
        , enableMaxZoom(false)
        , minZoom(0.1f)
        , maxZoom(10.0f)
        , enableBounds(false)
        , enableRotationLimits(false)
        , minRotation(-3.14159f)
        , maxRotation(3.14159f)
    {}
};

/**
 * @brief Camera shake preset
 */
struct CameraShakePreset {
    std::string name;
    float intensity;
    float duration;
    float frequency;
    bool decayOverTime;
    bool rotationalShake;
    float rotationIntensity;
    
    CameraShakePreset()
        : intensity(5.0f)
        , duration(0.5f)
        , frequency(20.0f)
        , decayOverTime(true)
        , rotationalShake(false)
        , rotationIntensity(0.05f)
    {}
};

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
    CameraFollowConfig followConfig;
    
    // Follow state (for advanced modes)
    Math::Vector2D velocity;
    Math::Vector2D springVelocity;
    Math::Vector2D lastTargetPosition;
    Math::Vector2D lookAheadOffset;
    
    // Bounds
    bool hasBounds;
    Math::Vector2D boundsMin;
    Math::Vector2D boundsMax;
    
    // Constraints
    CameraConstraints constraints;
    
    // Shake effect
    bool isShaking;
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;
    float shakeFrequency;
    bool shakeDecay;
    bool rotationalShake;
    float rotationShakeIntensity;
    Math::Vector2D shakeOffset;
    float shakeRotationOffset;
    
    // Shake presets
    std::unordered_map<std::string, CameraShakePreset> shakePresets;
    
    // Zoom animation
    bool isZooming;
    float zoomTarget;
    float zoomSpeed;
    float zoomDuration;
    float zoomTimer;
    
    // Path following
    struct PathPoint {
        Math::Vector2D position;
        float zoom;
        float rotation;
        float duration;
    };
    std::vector<PathPoint> cameraPath;
    size_t currentPathIndex;
    float pathTimer;
    bool isFollowingPath;
    bool loopPath;
    std::function<void()> onPathComplete;
    
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
    void zoomTo(float targetZoom, float duration);
    void zoomToFit(const Math::Vector2D& min, const Math::Vector2D& max, float padding = 0.0f);
    
    // Rotation
    void setRotation(float rot);
    void rotate(float delta);
    float getRotation() const { return rotation; }
    
    // Offset (for screen shake, etc.)
    void setOffset(const Math::Vector2D& off) { offset = off; }
    const Math::Vector2D& getOffset() const { return offset; }
    
    // Follow target with mode selection
    void setFollowTarget(ECS::Entity* target, float smoothing = 5.0f);
    void setFollowTarget(ECS::Entity* target, const CameraFollowConfig& config);
    void setFollowOffset(const Math::Vector2D& off) { followOffset = off; }
    void setFollowConfig(const CameraFollowConfig& config) { followConfig = config; }
    const CameraFollowConfig& getFollowConfig() const { return followConfig; }
    void clearFollowTarget();
    
    // Constraints
    void setConstraints(const CameraConstraints& constraints);
    const CameraConstraints& getConstraints() const { return constraints; }
    void clearConstraints();
    
    // Bounds (legacy, use constraints for new code)
    void setBounds(const Math::Vector2D& min, const Math::Vector2D& max);
    void clearBounds();
    
    // Camera shake
    void shake(float intensity, float duration);
    void shake(const CameraShakePreset& preset);
    void shakeWithFrequency(float intensity, float duration, float frequency);
    void stopShake();
    
    // Shake presets
    void registerShakePreset(const std::string& name, const CameraShakePreset& preset);
    void shake(const std::string& presetName);
    bool hasShakePreset(const std::string& name) const;
    void loadDefaultShakePresets();  // Load built-in presets (explosion, earthquake, rumble, etc.)
    CameraShakePreset getShakePreset(const std::string& name) const;
    
    // Camera path
    void addPathPoint(const Math::Vector2D& position, float zoom, float rotation, float duration);
    void clearPath();
    void startPath(bool loop = false, std::function<void()> onComplete = nullptr);
    void stopPath();
    void pausePath();
    void resumePath();
    bool isOnPath() const { return isFollowingPath; }
    float getPathProgress() const;
    
    // Utility
    bool isInView(const Math::Vector2D& point, float margin = 0.0f) const;
    bool isRectInView(const Math::Vector2D& pos, const Math::Vector2D& size) const;
    Math::Vector2D getCenter() const;
    
private:
    void updateFollow(float deltaTime);
    void updateFollowInstant();
    void updateFollowLinear(float deltaTime);
    void updateFollowSmoothDamp(float deltaTime);
    void updateFollowSpring(float deltaTime);
    void updateFollowPredictive(float deltaTime);
    void updateFollowDeadZone(float deltaTime);
    void updateFollowLookAhead(float deltaTime);
    
    void updateShake(float deltaTime);
    void updateZoom(float deltaTime);
    void updatePath(float deltaTime);
    void applyConstraints();
    
    Math::Vector2D smoothDamp(const Math::Vector2D& current, const Math::Vector2D& target,
                              Math::Vector2D& velocity, float smoothTime, float maxSpeed, float deltaTime);
};

} // namespace Graphics
} // namespace JJM

#endif // CAMERA_H
