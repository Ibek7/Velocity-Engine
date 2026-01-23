#ifndef GESTURE_RECOGNITION_H
#define GESTURE_RECOGNITION_H

#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <chrono>

namespace JJM {
namespace Input {

// Forward declarations
struct TouchPoint;
struct GestureEvent;
class GestureRecognizer;

// Touch point data
struct TouchPoint {
    int id;                              // Touch identifier
    float x, y;                          // Position
    float prevX, prevY;                  // Previous position
    std::chrono::steady_clock::time_point startTime;  // Touch start time
    std::chrono::steady_clock::time_point updateTime; // Last update time
    bool active;                         // Is touch active
    
    TouchPoint() : id(-1), x(0), y(0), prevX(0), prevY(0), active(false) {}
};

// Gesture types
enum class GestureType {
    NONE,
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    PINCH,
    ZOOM,
    ROTATE,
    PAN,
    MULTI_TAP,
    EDGE_SWIPE,
    CUSTOM
};

// Swipe direction
enum class SwipeDirection {
    NONE,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    UP_LEFT,
    UP_RIGHT,
    DOWN_LEFT,
    DOWN_RIGHT
};

// Gesture state
enum class GestureState {
    POSSIBLE,     // Gesture may be recognized
    BEGAN,        // Gesture has started
    CHANGED,      // Gesture parameters changed
    ENDED,        // Gesture completed successfully
    CANCELLED,    // Gesture was cancelled
    FAILED        // Gesture recognition failed
};

// Gesture event data
struct GestureEvent {
    GestureType type;
    GestureState state;
    
    // Common properties
    int touchCount;
    float centerX, centerY;              // Center of all touches
    float deltaX, deltaY;                // Movement delta
    float velocity;                      // Movement velocity
    
    // Tap properties
    int tapCount;
    
    // Swipe properties
    SwipeDirection swipeDirection;
    float swipeDistance;
    
    // Pinch/Zoom properties
    float scale;                         // Scale factor
    float scaleVelocity;
    
    // Rotation properties
    float rotation;                      // Rotation angle in radians
    float rotationVelocity;
    
    // Pan properties
    float translationX, translationY;
    
    // Timing
    double duration;                     // Gesture duration in seconds
    
    // User data
    void* userData;
    
    GestureEvent() 
        : type(GestureType::NONE)
        , state(GestureState::POSSIBLE)
        , touchCount(0)
        , centerX(0), centerY(0)
        , deltaX(0), deltaY(0)
        , velocity(0)
        , tapCount(0)
        , swipeDirection(SwipeDirection::NONE)
        , swipeDistance(0)
        , scale(1.0f)
        , scaleVelocity(0)
        , rotation(0)
        , rotationVelocity(0)
        , translationX(0), translationY(0)
        , duration(0)
        , userData(nullptr) {}
};

// Gesture callback
using GestureCallback = std::function<void(const GestureEvent&)>;

// Gesture recognizer configuration
struct GestureConfig {
    // Tap configuration
    float tapMaxDistance = 10.0f;        // Maximum movement for tap
    double tapMaxDuration = 0.3;         // Maximum duration for tap
    double doubleTapMaxDelay = 0.3;      // Maximum delay between taps
    
    // Long press configuration
    double longPressMinDuration = 0.5;   // Minimum duration for long press
    float longPressMaxDistance = 10.0f;  // Maximum movement for long press
    
    // Swipe configuration
    float swipeMinDistance = 50.0f;      // Minimum distance for swipe
    float swipeMinVelocity = 100.0f;     // Minimum velocity for swipe
    double swipeMaxDuration = 1.0;       // Maximum duration for swipe
    
    // Pinch/Zoom configuration
    float pinchMinScale = 0.5f;          // Minimum scale change
    float pinchMaxScale = 2.0f;          // Maximum scale change
    
    // Rotation configuration
    float rotationMinAngle = 5.0f;       // Minimum rotation angle (degrees)
    
    // Pan configuration
    float panMinDistance = 5.0f;         // Minimum distance to start pan
    
    // Edge swipe configuration
    float edgeSwipeMargin = 30.0f;       // Distance from edge
    
    // Multi-touch configuration
    int maxTouchPoints = 10;             // Maximum simultaneous touches
};

// Base gesture recognizer
class GestureRecognizer {
public:
    virtual ~GestureRecognizer() = default;
    
    // Configuration
    virtual void setConfig(const GestureConfig& config) { m_config = config; }
    virtual GestureConfig& getConfig() { return m_config; }
    
    // Recognition
    virtual void reset() = 0;
    virtual GestureState recognize(const std::vector<TouchPoint>& touches) = 0;
    virtual GestureEvent getEvent() const = 0;
    
    // State
    virtual GestureType getType() const = 0;
    virtual GestureState getState() const { return m_state; }
    virtual bool isRecognizing() const { return m_state != GestureState::POSSIBLE && 
                                                m_state != GestureState::FAILED; }
    
    // Callback
    void setCallback(GestureCallback callback) { m_callback = callback; }
    
    // Enable/disable
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
protected:
    GestureConfig m_config;
    GestureState m_state = GestureState::POSSIBLE;
    GestureCallback m_callback;
    bool m_enabled = true;
    
    void triggerCallback(const GestureEvent& event) {
        if (m_callback && m_enabled) {
            m_callback(event);
        }
    }
    
    // Helper functions
    float distance(float x1, float y1, float x2, float y2) const;
    float angle(float x1, float y1, float x2, float y2) const;
    void calculateCenter(const std::vector<TouchPoint>& touches, float& cx, float& cy) const;
    double getDuration(const std::chrono::steady_clock::time_point& start) const;
};

// Specific gesture recognizers
class TapRecognizer : public GestureRecognizer {
public:
    TapRecognizer(int requiredTaps = 1);
    
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::TAP; }
    
private:
    int m_requiredTaps;
    int m_currentTapCount;
    std::chrono::steady_clock::time_point m_lastTapTime;
    float m_startX, m_startY;
    GestureEvent m_event;
};

class LongPressRecognizer : public GestureRecognizer {
public:
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::LONG_PRESS; }
    
private:
    float m_startX, m_startY;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_pressed;
    GestureEvent m_event;
};

class SwipeRecognizer : public GestureRecognizer {
public:
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::SWIPE_LEFT; } // Dynamic
    
private:
    float m_startX, m_startY;
    std::chrono::steady_clock::time_point m_startTime;
    GestureEvent m_event;
    
    SwipeDirection determineDirection(float dx, float dy) const;
};

class PinchRecognizer : public GestureRecognizer {
public:
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::PINCH; }
    
private:
    float m_initialDistance;
    float m_previousDistance;
    float m_previousScale;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    GestureEvent m_event;
};

class RotationRecognizer : public GestureRecognizer {
public:
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::ROTATE; }
    
private:
    float m_initialAngle;
    float m_previousAngle;
    float m_previousRotation;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    GestureEvent m_event;
};

class PanRecognizer : public GestureRecognizer {
public:
    void reset() override;
    GestureState recognize(const std::vector<TouchPoint>& touches) override;
    GestureEvent getEvent() const override;
    GestureType getType() const override { return GestureType::PAN; }
    
private:
    float m_startX, m_startY;
    float m_previousX, m_previousY;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    bool m_started;
    GestureEvent m_event;
};

// Gesture recognition manager
class GestureRecognitionSystem {
public:
    GestureRecognitionSystem();
    ~GestureRecognitionSystem();
    
    // Recognizer management
    void addRecognizer(std::shared_ptr<GestureRecognizer> recognizer);
    void removeRecognizer(std::shared_ptr<GestureRecognizer> recognizer);
    void clearRecognizers();
    std::vector<std::shared_ptr<GestureRecognizer>>& getRecognizers() { return m_recognizers; }
    
    // Touch input
    void touchBegan(int touchId, float x, float y);
    void touchMoved(int touchId, float x, float y);
    void touchEnded(int touchId, float x, float y);
    void touchCancelled(int touchId);
    
    // Mouse input (simulates touch)
    void mouseDown(int button, float x, float y);
    void mouseMove(float x, float y);
    void mouseUp(int button, float x, float y);
    
    // Update
    void update(float deltaTime);
    
    // Configuration
    void setGlobalConfig(const GestureConfig& config);
    GestureConfig& getGlobalConfig() { return m_globalConfig; }
    
    // Utility
    const std::vector<TouchPoint>& getActiveTouches() const { return m_touches; }
    int getActiveTouchCount() const;
    
    // Quick setup helpers
    void setupCommonGestures();
    std::shared_ptr<TapRecognizer> addTapGesture(GestureCallback callback, int taps = 1);
    std::shared_ptr<LongPressRecognizer> addLongPressGesture(GestureCallback callback);
    std::shared_ptr<SwipeRecognizer> addSwipeGesture(GestureCallback callback);
    std::shared_ptr<PinchRecognizer> addPinchGesture(GestureCallback callback);
    std::shared_ptr<RotationRecognizer> addRotationGesture(GestureCallback callback);
    std::shared_ptr<PanRecognizer> addPanGesture(GestureCallback callback);
    
private:
    std::vector<std::shared_ptr<GestureRecognizer>> m_recognizers;
    std::vector<TouchPoint> m_touches;
    GestureConfig m_globalConfig;
    int m_nextTouchId;
    
    // Mouse state (for mouse-to-touch simulation)
    bool m_mouseDown[3]; // Left, Middle, Right
    float m_mouseX, m_mouseY;
    
    TouchPoint* findTouch(int touchId);
    void removeTouch(int touchId);
    void processRecognizers();
};

} // namespace Input
} // namespace JJM

#endif // GESTURE_RECOGNITION_H
