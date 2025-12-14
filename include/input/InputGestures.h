#pragma once

#include "input/InputManager.h"
#include "math/Vector2D.h"
#include <vector>
#include <functional>
#include <chrono>

namespace JJM {
namespace Input {

enum class GestureType {
    None,
    Tap,
    DoubleTap,
    LongPress,
    Swipe,
    Pinch,
    Rotate,
    Pan,
    Flick
};

enum class SwipeDirection {
    None,
    Up,
    Down,
    Left,
    Right
};

struct TouchPoint {
    int id;
    Math::Vector2D position;
    Math::Vector2D startPosition;
    std::chrono::steady_clock::time_point startTime;
    bool active;
};

class Gesture {
public:
    Gesture();
    virtual ~Gesture();
    
    virtual GestureType getType() const = 0;
    virtual bool recognize(const std::vector<TouchPoint>& touches) = 0;
    virtual void reset() = 0;
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    bool isRecognized() const { return recognized; }

protected:
    bool enabled;
    bool recognized;
};

class TapGesture : public Gesture {
public:
    TapGesture();
    ~TapGesture();
    
    GestureType getType() const override { return GestureType::Tap; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMaxDuration(float duration) { maxDuration = duration; }
    float getMaxDuration() const { return maxDuration; }
    
    void setMaxMovement(float movement) { maxMovement = movement; }
    float getMaxMovement() const { return maxMovement; }
    
    const Math::Vector2D& getTapPosition() const { return tapPosition; }

private:
    float maxDuration;
    float maxMovement;
    Math::Vector2D tapPosition;
};

class DoubleTapGesture : public Gesture {
public:
    DoubleTapGesture();
    ~DoubleTapGesture();
    
    GestureType getType() const override { return GestureType::DoubleTap; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMaxDelay(float delay) { maxDelay = delay; }
    float getMaxDelay() const { return maxDelay; }
    
    void setMaxDistance(float distance) { maxDistance = distance; }
    float getMaxDistance() const { return maxDistance; }
    
    const Math::Vector2D& getTapPosition() const { return tapPosition; }

private:
    float maxDelay;
    float maxDistance;
    Math::Vector2D tapPosition;
    std::chrono::steady_clock::time_point lastTapTime;
    int tapCount;
};

class LongPressGesture : public Gesture {
public:
    LongPressGesture();
    ~LongPressGesture();
    
    GestureType getType() const override { return GestureType::LongPress; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMinDuration(float duration) { minDuration = duration; }
    float getMinDuration() const { return minDuration; }
    
    void setMaxMovement(float movement) { maxMovement = movement; }
    float getMaxMovement() const { return maxMovement; }
    
    const Math::Vector2D& getPressPosition() const { return pressPosition; }

private:
    float minDuration;
    float maxMovement;
    Math::Vector2D pressPosition;
    std::chrono::steady_clock::time_point startTime;
    bool pressing;
};

class SwipeGesture : public Gesture {
public:
    SwipeGesture();
    ~SwipeGesture();
    
    GestureType getType() const override { return GestureType::Swipe; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMinDistance(float distance) { minDistance = distance; }
    float getMinDistance() const { return minDistance; }
    
    void setMaxDuration(float duration) { maxDuration = duration; }
    float getMaxDuration() const { return maxDuration; }
    
    SwipeDirection getDirection() const { return direction; }
    const Math::Vector2D& getStartPosition() const { return startPosition; }
    const Math::Vector2D& getEndPosition() const { return endPosition; }
    float getVelocity() const { return velocity; }

private:
    float minDistance;
    float maxDuration;
    SwipeDirection direction;
    Math::Vector2D startPosition;
    Math::Vector2D endPosition;
    float velocity;
};

class PinchGesture : public Gesture {
public:
    PinchGesture();
    ~PinchGesture();
    
    GestureType getType() const override { return GestureType::Pinch; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    float getScale() const { return scale; }
    const Math::Vector2D& getCenter() const { return center; }

private:
    float scale;
    float initialDistance;
    Math::Vector2D center;
};

class RotateGesture : public Gesture {
public:
    RotateGesture();
    ~RotateGesture();
    
    GestureType getType() const override { return GestureType::Rotate; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    float getRotation() const { return rotation; }
    const Math::Vector2D& getCenter() const { return center; }

private:
    float rotation;
    float initialAngle;
    Math::Vector2D center;
};

class PanGesture : public Gesture {
public:
    PanGesture();
    ~PanGesture();
    
    GestureType getType() const override { return GestureType::Pan; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMinTouches(int count) { minTouches = count; }
    int getMinTouches() const { return minTouches; }
    
    const Math::Vector2D& getDelta() const { return delta; }
    const Math::Vector2D& getPosition() const { return position; }

private:
    int minTouches;
    Math::Vector2D delta;
    Math::Vector2D position;
    Math::Vector2D lastPosition;
};

class FlickGesture : public Gesture {
public:
    FlickGesture();
    ~FlickGesture();
    
    GestureType getType() const override { return GestureType::Flick; }
    bool recognize(const std::vector<TouchPoint>& touches) override;
    void reset() override;
    
    void setMinVelocity(float velocity) { minVelocity = velocity; }
    float getMinVelocity() const { return minVelocity; }
    
    const Math::Vector2D& getDirection() const { return direction; }
    float getVelocity() const { return velocity; }

private:
    float minVelocity;
    Math::Vector2D direction;
    float velocity;
    std::chrono::steady_clock::time_point lastTime;
    Math::Vector2D lastPosition;
};

class GestureRecognizer {
public:
    GestureRecognizer();
    ~GestureRecognizer();
    
    void update(InputManager* input);
    
    void addGesture(std::shared_ptr<Gesture> gesture);
    void removeGesture(GestureType type);
    void clearGestures();
    
    std::shared_ptr<Gesture> getGesture(GestureType type);
    
    void setOnGestureRecognized(std::function<void(GestureType)> callback) {
        onGestureRecognized = callback;
    }

private:
    std::vector<std::shared_ptr<Gesture>> gestures;
    std::vector<TouchPoint> touches;
    std::function<void(GestureType)> onGestureRecognized;
    
    void updateTouches(InputManager* input);
};

} // namespace Input
} // namespace JJM
