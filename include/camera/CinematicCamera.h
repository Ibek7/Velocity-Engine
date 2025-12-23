#ifndef JJM_CINEMATIC_CAMERA_H
#define JJM_CINEMATIC_CAMERA_H

#include "math/Vector2D.h"
#include <vector>
#include <functional>

namespace JJM {
namespace Math {
struct Vector3D {
    float x, y, z;
    Vector3D(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
};
}
}

namespace JJM {
namespace Camera {

struct CameraKeyframe {
    Math::Vector3D position;
    Math::Vector3D target;
    float time;
    float fov;
};

enum class InterpolationType { Linear, Smooth, Bezier };

class CinematicCamera {
public:
    CinematicCamera();
    ~CinematicCamera();
    
    void addKeyframe(const CameraKeyframe& keyframe);
    void clearKeyframes();
    
    void play();
    void pause();
    void stop();
    void update(float deltaTime);
    
    bool isPlaying() const;
    float getCurrentTime() const;
    float getDuration() const;
    
    Math::Vector3D getPosition() const;
    Math::Vector3D getTarget() const;
    float getFOV() const;
    
    void setInterpolationType(InterpolationType type);
    void setLoop(bool loop);
    
    using OnCompleteCallback = std::function<void()>;
    void setOnCompleteCallback(OnCompleteCallback callback);

private:
    std::vector<CameraKeyframe> keyframes;
    float currentTime;
    bool playing;
    bool looping;
    InterpolationType interpolation;
    OnCompleteCallback onComplete;
    
    CameraKeyframe interpolate(const CameraKeyframe& a, const CameraKeyframe& b, float t);
    Math::Vector3D lerpVector(const Math::Vector3D& a, const Math::Vector3D& b, float t) const;
    float smoothstep(float t) const;
};

} // namespace Camera
} // namespace JJM

#endif
