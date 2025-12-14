#include "input/InputGestures.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Input {

// Gesture implementation
Gesture::Gesture() : enabled(true), recognized(false) {}

Gesture::~Gesture() {}

// TapGesture implementation
TapGesture::TapGesture() : maxDuration(0.3f), maxMovement(10.0f) {}

TapGesture::~TapGesture() {}

bool TapGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 1) {
        return false;
    }
    
    const TouchPoint& touch = touches[0];
    if (!touch.active) {
        auto duration = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - touch.startTime).count();
        
        float distance = std::sqrt(
            std::pow(touch.position.x - touch.startPosition.x, 2) +
            std::pow(touch.position.y - touch.startPosition.y, 2));
        
        if (duration <= maxDuration && distance <= maxMovement) {
            tapPosition = touch.position;
            recognized = true;
            return true;
        }
    }
    
    return false;
}

void TapGesture::reset() {
    recognized = false;
}

// DoubleTapGesture implementation
DoubleTapGesture::DoubleTapGesture()
    : maxDelay(0.5f), maxDistance(50.0f), tapCount(0) {}

DoubleTapGesture::~DoubleTapGesture() {}

bool DoubleTapGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 1) {
        return false;
    }
    
    const TouchPoint& touch = touches[0];
    if (!touch.active) {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceLastTap = std::chrono::duration<float>(now - lastTapTime).count();
        
        if (tapCount == 0 || timeSinceLastTap <= maxDelay) {
            float distance = std::sqrt(
                std::pow(touch.position.x - tapPosition.x, 2) +
                std::pow(touch.position.y - tapPosition.y, 2));
            
            if (tapCount == 0 || distance <= maxDistance) {
                tapCount++;
                lastTapTime = now;
                tapPosition = touch.position;
                
                if (tapCount == 2) {
                    recognized = true;
                    tapCount = 0;
                    return true;
                }
            }
        } else {
            tapCount = 0;
        }
    }
    
    return false;
}

void DoubleTapGesture::reset() {
    recognized = false;
    tapCount = 0;
}

// LongPressGesture implementation
LongPressGesture::LongPressGesture()
    : minDuration(0.5f), maxMovement(10.0f), pressing(false) {}

LongPressGesture::~LongPressGesture() {}

bool LongPressGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 1) {
        pressing = false;
        return false;
    }
    
    const TouchPoint& touch = touches[0];
    if (touch.active) {
        if (!pressing) {
            pressing = true;
            startTime = touch.startTime;
            pressPosition = touch.startPosition;
        }
        
        auto duration = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - startTime).count();
        
        float distance = std::sqrt(
            std::pow(touch.position.x - pressPosition.x, 2) +
            std::pow(touch.position.y - pressPosition.y, 2));
        
        if (duration >= minDuration && distance <= maxMovement) {
            recognized = true;
            return true;
        }
    } else {
        pressing = false;
    }
    
    return false;
}

void LongPressGesture::reset() {
    recognized = false;
    pressing = false;
}

// SwipeGesture implementation
SwipeGesture::SwipeGesture()
    : minDistance(50.0f), maxDuration(0.5f),
      direction(SwipeDirection::None), velocity(0.0f) {}

SwipeGesture::~SwipeGesture() {}

bool SwipeGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 1) {
        return false;
    }
    
    const TouchPoint& touch = touches[0];
    if (!touch.active) {
        auto duration = std::chrono::duration<float>(
            std::chrono::steady_clock::now() - touch.startTime).count();
        
        Math::Vector2D delta(
            touch.position.x - touch.startPosition.x,
            touch.position.y - touch.startPosition.y);
        
        float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        
        if (distance >= minDistance && duration <= maxDuration) {
            startPosition = touch.startPosition;
            endPosition = touch.position;
            velocity = distance / duration;
            
            // Determine direction
            if (std::abs(delta.x) > std::abs(delta.y)) {
                direction = delta.x > 0 ? SwipeDirection::Right : SwipeDirection::Left;
            } else {
                direction = delta.y > 0 ? SwipeDirection::Down : SwipeDirection::Up;
            }
            
            recognized = true;
            return true;
        }
    }
    
    return false;
}

void SwipeGesture::reset() {
    recognized = false;
    direction = SwipeDirection::None;
    velocity = 0.0f;
}

// PinchGesture implementation
PinchGesture::PinchGesture() : scale(1.0f), initialDistance(0.0f) {}

PinchGesture::~PinchGesture() {}

bool PinchGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 2) {
        initialDistance = 0.0f;
        return false;
    }
    
    Math::Vector2D delta(
        touches[1].position.x - touches[0].position.x,
        touches[1].position.y - touches[0].position.y);
    
    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
    
    if (initialDistance == 0.0f) {
        initialDistance = distance;
    }
    
    if (initialDistance > 0.0f) {
        scale = distance / initialDistance;
        center.x = (touches[0].position.x + touches[1].position.x) / 2.0f;
        center.y = (touches[0].position.y + touches[1].position.y) / 2.0f;
        recognized = true;
        return true;
    }
    
    return false;
}

void PinchGesture::reset() {
    recognized = false;
    scale = 1.0f;
    initialDistance = 0.0f;
}

// RotateGesture implementation
RotateGesture::RotateGesture() : rotation(0.0f), initialAngle(0.0f) {}

RotateGesture::~RotateGesture() {}

bool RotateGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 2) {
        initialAngle = 0.0f;
        return false;
    }
    
    Math::Vector2D delta(
        touches[1].position.x - touches[0].position.x,
        touches[1].position.y - touches[0].position.y);
    
    float angle = std::atan2(delta.y, delta.x);
    
    if (initialAngle == 0.0f) {
        initialAngle = angle;
    }
    
    rotation = angle - initialAngle;
    center.x = (touches[0].position.x + touches[1].position.x) / 2.0f;
    center.y = (touches[0].position.y + touches[1].position.y) / 2.0f;
    recognized = true;
    return true;
}

void RotateGesture::reset() {
    recognized = false;
    rotation = 0.0f;
    initialAngle = 0.0f;
}

// PanGesture implementation
PanGesture::PanGesture() : minTouches(1) {}

PanGesture::~PanGesture() {}

bool PanGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() < static_cast<size_t>(minTouches)) {
        return false;
    }
    
    // Calculate average position
    Math::Vector2D avgPos(0.0f, 0.0f);
    for (const auto& touch : touches) {
        if (touch.active) {
            avgPos.x += touch.position.x;
            avgPos.y += touch.position.y;
        }
    }
    avgPos.x /= touches.size();
    avgPos.y /= touches.size();
    
    if (lastPosition.x != 0.0f || lastPosition.y != 0.0f) {
        delta.x = avgPos.x - lastPosition.x;
        delta.y = avgPos.y - lastPosition.y;
    }
    
    lastPosition = avgPos;
    position = avgPos;
    recognized = true;
    return true;
}

void PanGesture::reset() {
    recognized = false;
    delta = Math::Vector2D(0.0f, 0.0f);
    lastPosition = Math::Vector2D(0.0f, 0.0f);
}

// FlickGesture implementation
FlickGesture::FlickGesture() : minVelocity(500.0f), velocity(0.0f) {}

FlickGesture::~FlickGesture() {}

bool FlickGesture::recognize(const std::vector<TouchPoint>& touches) {
    if (!enabled || touches.size() != 1) {
        return false;
    }
    
    const TouchPoint& touch = touches[0];
    auto now = std::chrono::steady_clock::now();
    
    if (touch.active) {
        if (lastTime.time_since_epoch().count() != 0) {
            auto dt = std::chrono::duration<float>(now - lastTime).count();
            if (dt > 0.0f) {
                Math::Vector2D delta(
                    touch.position.x - lastPosition.x,
                    touch.position.y - lastPosition.y);
                
                float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);
                velocity = distance / dt;
                
                if (velocity >= minVelocity) {
                    direction.x = delta.x / distance;
                    direction.y = delta.y / distance;
                    recognized = true;
                    return true;
                }
            }
        }
        
        lastTime = now;
        lastPosition = touch.position;
    }
    
    return false;
}

void FlickGesture::reset() {
    recognized = false;
    velocity = 0.0f;
    lastTime = std::chrono::steady_clock::time_point();
}

// GestureRecognizer implementation
GestureRecognizer::GestureRecognizer() {}

GestureRecognizer::~GestureRecognizer() {}

void GestureRecognizer::update(InputManager* input) {
    if (!input) return;
    
    updateTouches(input);
    
    for (auto& gesture : gestures) {
        if (gesture && gesture->isEnabled()) {
            if (gesture->recognize(touches)) {
                if (onGestureRecognized) {
                    onGestureRecognized(gesture->getType());
                }
            }
        }
    }
}

void GestureRecognizer::addGesture(std::shared_ptr<Gesture> gesture) {
    if (gesture) {
        gestures.push_back(gesture);
    }
}

void GestureRecognizer::removeGesture(GestureType type) {
    gestures.erase(
        std::remove_if(gestures.begin(), gestures.end(),
            [type](const std::shared_ptr<Gesture>& gesture) {
                return gesture->getType() == type;
            }),
        gestures.end());
}

void GestureRecognizer::clearGestures() {
    gestures.clear();
}

std::shared_ptr<Gesture> GestureRecognizer::getGesture(GestureType type) {
    for (auto& gesture : gestures) {
        if (gesture->getType() == type) {
            return gesture;
        }
    }
    return nullptr;
}

void GestureRecognizer::updateTouches(InputManager* input) {
    // This would need to be implemented based on InputManager's touch API
    // For now, just a stub
}

} // namespace Input
} // namespace JJM
