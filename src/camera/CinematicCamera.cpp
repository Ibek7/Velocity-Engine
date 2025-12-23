#include "camera/CinematicCamera.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace JJM {
namespace Camera {

CinematicCamera::CinematicCamera()
    : currentTime(0.0f), playing(false), looping(false), 
      interpolation(InterpolationType::Smooth) {
}

CinematicCamera::~CinematicCamera() {
}

void CinematicCamera::addKeyframe(const CameraKeyframe& keyframe) {
    keyframes.push_back(keyframe);
    std::sort(keyframes.begin(), keyframes.end(),
        [](const CameraKeyframe& a, const CameraKeyframe& b) {
            return a.time < b.time;
        });
}

void CinematicCamera::clearKeyframes() {
    keyframes.clear();
}

void CinematicCamera::play() {
    if (keyframes.empty()) return;
    playing = true;
    std::cout << "Cinematic camera started" << std::endl;
}

void CinematicCamera::pause() {
    playing = false;
}

void CinematicCamera::stop() {
    playing = false;
    currentTime = 0.0f;
}

void CinematicCamera::update(float deltaTime) {
    if (!playing || keyframes.empty()) return;
    
    currentTime += deltaTime;
    
    float duration = getDuration();
    if (currentTime >= duration) {
        if (looping) {
            currentTime = 0.0f;
        } else {
            playing = false;
            if (onComplete) {
                onComplete();
            }
        }
    }
}

bool CinematicCamera::isPlaying() const {
    return playing;
}

float CinematicCamera::getCurrentTime() const {
    return currentTime;
}

float CinematicCamera::getDuration() const {
    if (keyframes.empty()) return 0.0f;
    return keyframes.back().time;
}

Math::Vector3D CinematicCamera::getPosition() const {
    if (keyframes.empty()) return Math::Vector3D(0, 0, 0);
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (currentTime >= keyframes[i].time && currentTime <= keyframes[i + 1].time) {
            float t = (currentTime - keyframes[i].time) / 
                     (keyframes[i + 1].time - keyframes[i].time);
            
            if (interpolation == InterpolationType::Smooth) {
                t = smoothstep(t);
            }
            
            return lerpVector(keyframes[i].position, keyframes[i + 1].position, t);
        }
    }
    
    return keyframes.back().position;
}

Math::Vector3D CinematicCamera::getTarget() const {
    if (keyframes.empty()) return Math::Vector3D(0, 0, 0);
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (currentTime >= keyframes[i].time && currentTime <= keyframes[i + 1].time) {
            float t = (currentTime - keyframes[i].time) / 
                     (keyframes[i + 1].time - keyframes[i].time);
            
            if (interpolation == InterpolationType::Smooth) {
                t = smoothstep(t);
            }
            
            return lerpVector(keyframes[i].target, keyframes[i + 1].target, t);
        }
    }
    
    return keyframes.back().target;
}

float CinematicCamera::getFOV() const {
    if (keyframes.empty()) return 60.0f;
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (currentTime >= keyframes[i].time && currentTime <= keyframes[i + 1].time) {
            float t = (currentTime - keyframes[i].time) / 
                     (keyframes[i + 1].time - keyframes[i].time);
            
            if (interpolation == InterpolationType::Smooth) {
                t = smoothstep(t);
            }
            
            return keyframes[i].fov + (keyframes[i + 1].fov - keyframes[i].fov) * t;
        }
    }
    
    return keyframes.back().fov;
}

void CinematicCamera::setInterpolationType(InterpolationType type) {
    interpolation = type;
}

void CinematicCamera::setLoop(bool loop) {
    looping = loop;
}

void CinematicCamera::setOnCompleteCallback(OnCompleteCallback callback) {
    onComplete = callback;
}

CameraKeyframe CinematicCamera::interpolate(const CameraKeyframe& a, 
                                            const CameraKeyframe& b, float t) {
    CameraKeyframe result;
    result.position = lerpVector(a.position, b.position, t);
    result.target = lerpVector(a.target, b.target, t);
    result.fov = a.fov + (b.fov - a.fov) * t;
    result.time = a.time + (b.time - a.time) * t;
    return result;
}

Math::Vector3D CinematicCamera::lerpVector(const Math::Vector3D& a, 
                                          const Math::Vector3D& b, float t) const {
    return Math::Vector3D(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t
    );
}

float CinematicCamera::smoothstep(float t) const {
    return t * t * (3.0f - 2.0f * t);
}

} // namespace Camera
} // namespace JJM
