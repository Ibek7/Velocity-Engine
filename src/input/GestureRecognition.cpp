#include "../../include/input/GestureRecognition.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Input {

// ============================================================================
// GestureRecognizer Base Class
// ============================================================================

float GestureRecognizer::distance(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

float GestureRecognizer::angle(float x1, float y1, float x2, float y2) const {
    return std::atan2(y2 - y1, x2 - x1);
}

void GestureRecognizer::calculateCenter(const std::vector<TouchPoint>& touches, 
                                       float& cx, float& cy) const {
    cx = cy = 0;
    int count = 0;
    for (const auto& touch : touches) {
        if (touch.active) {
            cx += touch.x;
            cy += touch.y;
            count++;
        }
    }
    if (count > 0) {
        cx /= count;
        cy /= count;
    }
}

double GestureRecognizer::getDuration(const std::chrono::steady_clock::time_point& start) const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(now - start).count();
}

// ============================================================================
// TapRecognizer
// ============================================================================

TapRecognizer::TapRecognizer(int requiredTaps)
    : m_requiredTaps(requiredTaps)
    , m_currentTapCount(0)
    , m_startX(0), m_startY(0) {
}

void TapRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_currentTapCount = 0;
    m_event = GestureEvent();
}

GestureState TapRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    // Single touch only
    int activeCount = 0;
    const TouchPoint* activeTouch = nullptr;
    for (const auto& touch : touches) {
        if (touch.active) {
            activeCount++;
            activeTouch = &touch;
        }
    }
    
    if (activeCount != 1) {
        if (m_state == GestureState::BEGAN || m_state == GestureState::CHANGED) {
            m_state = GestureState::CANCELLED;
            m_event.state = m_state;
            triggerCallback(m_event);
        }
        return m_state;
    }
    
    if (m_state == GestureState::POSSIBLE && activeTouch) {
        // Touch started
        m_startX = activeTouch->x;
        m_startY = activeTouch->y;
        m_state = GestureState::BEGAN;
    } else if (m_state == GestureState::BEGAN && activeTouch) {
        // Check if movement is within threshold
        float dist = distance(m_startX, m_startY, activeTouch->x, activeTouch->y);
        double duration = getDuration(activeTouch->startTime);
        
        if (dist > m_config.tapMaxDistance) {
            m_state = GestureState::FAILED;
        } else if (duration > m_config.tapMaxDuration) {
            m_state = GestureState::FAILED;
        }
    }
    
    // Check for touch ended
    if (!activeTouch && m_state == GestureState::BEGAN) {
        // Touch ended, increment tap count
        auto now = std::chrono::steady_clock::now();
        double timeSinceLastTap = m_currentTapCount > 0 ? 
            std::chrono::duration<double>(now - m_lastTapTime).count() : 0.0;
        
        if (m_currentTapCount == 0 || timeSinceLastTap <= m_config.doubleTapMaxDelay) {
            m_currentTapCount++;
            m_lastTapTime = now;
            
            if (m_currentTapCount >= m_requiredTaps) {
                // Gesture recognized
                m_state = GestureState::ENDED;
                m_event.type = m_requiredTaps == 1 ? GestureType::TAP : 
                              m_requiredTaps == 2 ? GestureType::DOUBLE_TAP : 
                              GestureType::MULTI_TAP;
                m_event.state = m_state;
                m_event.centerX = m_startX;
                m_event.centerY = m_startY;
                m_event.tapCount = m_currentTapCount;
                m_event.touchCount = 1;
                triggerCallback(m_event);
                
                m_currentTapCount = 0;
                m_state = GestureState::POSSIBLE;
            } else {
                m_state = GestureState::POSSIBLE;
            }
        } else {
            // Too much time between taps
            m_currentTapCount = 0;
            m_state = GestureState::POSSIBLE;
        }
    }
    
    return m_state;
}

GestureEvent TapRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// LongPressRecognizer
// ============================================================================

void LongPressRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_pressed = false;
    m_event = GestureEvent();
}

GestureState LongPressRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    int activeCount = 0;
    const TouchPoint* activeTouch = nullptr;
    for (const auto& touch : touches) {
        if (touch.active) {
            activeCount++;
            activeTouch = &touch;
        }
    }
    
    if (activeCount == 1 && activeTouch) {
        if (!m_pressed) {
            // Touch started
            m_startX = activeTouch->x;
            m_startY = activeTouch->y;
            m_startTime = activeTouch->startTime;
            m_pressed = true;
            m_state = GestureState::POSSIBLE;
        } else {
            // Check duration and movement
            float dist = distance(m_startX, m_startY, activeTouch->x, activeTouch->y);
            double duration = getDuration(m_startTime);
            
            if (dist > m_config.longPressMaxDistance) {
                m_state = GestureState::FAILED;
                m_pressed = false;
            } else if (duration >= m_config.longPressMinDuration) {
                if (m_state != GestureState::BEGAN && m_state != GestureState::CHANGED) {
                    m_state = GestureState::BEGAN;
                    m_event.type = GestureType::LONG_PRESS;
                    m_event.state = m_state;
                    m_event.centerX = activeTouch->x;
                    m_event.centerY = activeTouch->y;
                    m_event.touchCount = 1;
                    m_event.duration = duration;
                    triggerCallback(m_event);
                } else {
                    m_state = GestureState::CHANGED;
                    m_event.state = m_state;
                    m_event.duration = duration;
                    triggerCallback(m_event);
                }
            }
        }
    } else if (m_pressed) {
        // Touch ended
        if (m_state == GestureState::BEGAN || m_state == GestureState::CHANGED) {
            m_state = GestureState::ENDED;
            m_event.state = m_state;
            triggerCallback(m_event);
        }
        m_pressed = false;
        m_state = GestureState::POSSIBLE;
    }
    
    return m_state;
}

GestureEvent LongPressRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// SwipeRecognizer
// ============================================================================

void SwipeRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_event = GestureEvent();
}

SwipeDirection SwipeRecognizer::determineDirection(float dx, float dy) const {
    float absDx = std::abs(dx);
    float absDy = std::abs(dy);
    
    // Determine primary direction
    if (absDx > absDy * 2) {
        return dx > 0 ? SwipeDirection::RIGHT : SwipeDirection::LEFT;
    } else if (absDy > absDx * 2) {
        return dy > 0 ? SwipeDirection::DOWN : SwipeDirection::UP;
    } else {
        // Diagonal
        if (dx > 0 && dy > 0) return SwipeDirection::DOWN_RIGHT;
        if (dx > 0 && dy < 0) return SwipeDirection::UP_RIGHT;
        if (dx < 0 && dy > 0) return SwipeDirection::DOWN_LEFT;
        return SwipeDirection::UP_LEFT;
    }
}

GestureState SwipeRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    int activeCount = 0;
    const TouchPoint* activeTouch = nullptr;
    for (const auto& touch : touches) {
        if (touch.active) {
            activeCount++;
            activeTouch = &touch;
        }
    }
    
    if (activeCount == 1 && activeTouch) {
        if (m_state == GestureState::POSSIBLE) {
            m_startX = activeTouch->x;
            m_startY = activeTouch->y;
            m_startTime = activeTouch->startTime;
            m_state = GestureState::BEGAN;
        }
    } else if (m_state == GestureState::BEGAN) {
        // Touch ended, check if it's a swipe
        float dx = activeTouch ? activeTouch->x - m_startX : 0;
        float dy = activeTouch ? activeTouch->y - m_startY : 0;
        float dist = std::sqrt(dx * dx + dy * dy);
        double duration = getDuration(m_startTime);
        float velocity = duration > 0 ? dist / duration : 0;
        
        if (dist >= m_config.swipeMinDistance && 
            velocity >= m_config.swipeMinVelocity &&
            duration <= m_config.swipeMaxDuration) {
            // Valid swipe
            m_state = GestureState::ENDED;
            m_event.type = GestureType::SWIPE_LEFT; // Will be updated based on direction
            m_event.state = m_state;
            m_event.swipeDirection = determineDirection(dx, dy);
            m_event.swipeDistance = dist;
            m_event.velocity = velocity;
            m_event.deltaX = dx;
            m_event.deltaY = dy;
            m_event.duration = duration;
            m_event.touchCount = 1;
            triggerCallback(m_event);
        }
        
        m_state = GestureState::POSSIBLE;
    }
    
    return m_state;
}

GestureEvent SwipeRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// PinchRecognizer
// ============================================================================

void PinchRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_initialDistance = 0;
    m_previousDistance = 0;
    m_previousScale = 1.0f;
    m_event = GestureEvent();
}

GestureState PinchRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    // Need exactly 2 touches
    int activeCount = 0;
    const TouchPoint* touch1 = nullptr;
    const TouchPoint* touch2 = nullptr;
    
    for (const auto& touch : touches) {
        if (touch.active) {
            if (!touch1) touch1 = &touch;
            else if (!touch2) touch2 = &touch;
            activeCount++;
        }
    }
    
    if (activeCount == 2 && touch1 && touch2) {
        float dist = distance(touch1->x, touch1->y, touch2->x, touch2->y);
        
        if (m_state == GestureState::POSSIBLE) {
            m_initialDistance = dist;
            m_previousDistance = dist;
            m_previousScale = 1.0f;
            m_state = GestureState::BEGAN;
            m_lastUpdateTime = std::chrono::steady_clock::now();
            
            m_event.type = GestureType::PINCH;
            m_event.state = m_state;
            m_event.scale = 1.0f;
            m_event.touchCount = 2;
            calculateCenter(touches, m_event.centerX, m_event.centerY);
            triggerCallback(m_event);
        } else {
            float scale = dist / m_initialDistance;
            auto now = std::chrono::steady_clock::now();
            double deltaTime = std::chrono::duration<double>(now - m_lastUpdateTime).count();
            float scaleVelocity = deltaTime > 0 ? (scale - m_previousScale) / deltaTime : 0;
            
            m_state = GestureState::CHANGED;
            m_event.state = m_state;
            m_event.scale = scale;
            m_event.scaleVelocity = scaleVelocity;
            calculateCenter(touches, m_event.centerX, m_event.centerY);
            triggerCallback(m_event);
            
            m_previousDistance = dist;
            m_previousScale = scale;
            m_lastUpdateTime = now;
        }
    } else if ((m_state == GestureState::BEGAN || m_state == GestureState::CHANGED) && 
               activeCount != 2) {
        m_state = GestureState::ENDED;
        m_event.state = m_state;
        triggerCallback(m_event);
        m_state = GestureState::POSSIBLE;
    }
    
    return m_state;
}

GestureEvent PinchRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// RotationRecognizer
// ============================================================================

void RotationRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_initialAngle = 0;
    m_previousAngle = 0;
    m_previousRotation = 0;
    m_event = GestureEvent();
}

GestureState RotationRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    int activeCount = 0;
    const TouchPoint* touch1 = nullptr;
    const TouchPoint* touch2 = nullptr;
    
    for (const auto& touch : touches) {
        if (touch.active) {
            if (!touch1) touch1 = &touch;
            else if (!touch2) touch2 = &touch;
            activeCount++;
        }
    }
    
    if (activeCount == 2 && touch1 && touch2) {
        float ang = angle(touch1->x, touch1->y, touch2->x, touch2->y);
        
        if (m_state == GestureState::POSSIBLE) {
            m_initialAngle = ang;
            m_previousAngle = ang;
            m_previousRotation = 0;
            m_state = GestureState::BEGAN;
            m_lastUpdateTime = std::chrono::steady_clock::now();
            
            m_event.type = GestureType::ROTATE;
            m_event.state = m_state;
            m_event.rotation = 0;
            m_event.touchCount = 2;
            calculateCenter(touches, m_event.centerX, m_event.centerY);
            triggerCallback(m_event);
        } else {
            float rotation = ang - m_initialAngle;
            
            // Normalize to [-PI, PI]
            while (rotation > M_PI) rotation -= 2 * M_PI;
            while (rotation < -M_PI) rotation += 2 * M_PI;
            
            auto now = std::chrono::steady_clock::now();
            double deltaTime = std::chrono::duration<double>(now - m_lastUpdateTime).count();
            float rotationVelocity = deltaTime > 0 ? 
                (rotation - m_previousRotation) / deltaTime : 0;
            
            m_state = GestureState::CHANGED;
            m_event.state = m_state;
            m_event.rotation = rotation;
            m_event.rotationVelocity = rotationVelocity;
            calculateCenter(touches, m_event.centerX, m_event.centerY);
            triggerCallback(m_event);
            
            m_previousAngle = ang;
            m_previousRotation = rotation;
            m_lastUpdateTime = now;
        }
    } else if ((m_state == GestureState::BEGAN || m_state == GestureState::CHANGED) && 
               activeCount != 2) {
        m_state = GestureState::ENDED;
        m_event.state = m_state;
        triggerCallback(m_event);
        m_state = GestureState::POSSIBLE;
    }
    
    return m_state;
}

GestureEvent RotationRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// PanRecognizer
// ============================================================================

void PanRecognizer::reset() {
    m_state = GestureState::POSSIBLE;
    m_started = false;
    m_event = GestureEvent();
}

GestureState PanRecognizer::recognize(const std::vector<TouchPoint>& touches) {
    if (!m_enabled) return m_state;
    
    int activeCount = 0;
    float centerX = 0, centerY = 0;
    
    for (const auto& touch : touches) {
        if (touch.active) {
            centerX += touch.x;
            centerY += touch.y;
            activeCount++;
        }
    }
    
    if (activeCount > 0) {
        centerX /= activeCount;
        centerY /= activeCount;
        
        if (!m_started) {
            m_startX = centerX;
            m_startY = centerY;
            m_previousX = centerX;
            m_previousY = centerY;
            m_startTime = std::chrono::steady_clock::now();
            m_lastUpdateTime = m_startTime;
            m_started = true;
            m_state = GestureState::POSSIBLE;
        } else {
            float totalDist = distance(m_startX, m_startY, centerX, centerY);
            
            if (totalDist >= m_config.panMinDistance || 
                m_state == GestureState::BEGAN || 
                m_state == GestureState::CHANGED) {
                
                auto now = std::chrono::steady_clock::now();
                double deltaTime = std::chrono::duration<double>(now - m_lastUpdateTime).count();
                float dx = centerX - m_previousX;
                float dy = centerY - m_previousY;
                float velocity = deltaTime > 0 ? 
                    distance(0, 0, dx, dy) / deltaTime : 0;
                
                if (m_state == GestureState::POSSIBLE) {
                    m_state = GestureState::BEGAN;
                } else {
                    m_state = GestureState::CHANGED;
                }
                
                m_event.type = GestureType::PAN;
                m_event.state = m_state;
                m_event.centerX = centerX;
                m_event.centerY = centerY;
                m_event.deltaX = dx;
                m_event.deltaY = dy;
                m_event.translationX = centerX - m_startX;
                m_event.translationY = centerY - m_startY;
                m_event.velocity = velocity;
                m_event.touchCount = activeCount;
                m_event.duration = getDuration(m_startTime);
                triggerCallback(m_event);
                
                m_previousX = centerX;
                m_previousY = centerY;
                m_lastUpdateTime = now;
            }
        }
    } else if (m_started) {
        // All touches ended
        if (m_state == GestureState::BEGAN || m_state == GestureState::CHANGED) {
            m_state = GestureState::ENDED;
            m_event.state = m_state;
            triggerCallback(m_event);
        }
        m_started = false;
        m_state = GestureState::POSSIBLE;
    }
    
    return m_state;
}

GestureEvent PanRecognizer::getEvent() const {
    return m_event;
}

// ============================================================================
// GestureRecognitionSystem
// ============================================================================

GestureRecognitionSystem::GestureRecognitionSystem()
    : m_nextTouchId(0)
    , m_mouseX(0), m_mouseY(0) {
    m_mouseDown[0] = m_mouseDown[1] = m_mouseDown[2] = false;
}

GestureRecognitionSystem::~GestureRecognitionSystem() {
}

void GestureRecognitionSystem::addRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    m_recognizers.push_back(recognizer);
    recognizer->setConfig(m_globalConfig);
}

void GestureRecognitionSystem::removeRecognizer(std::shared_ptr<GestureRecognizer> recognizer) {
    m_recognizers.erase(
        std::remove(m_recognizers.begin(), m_recognizers.end(), recognizer),
        m_recognizers.end()
    );
}

void GestureRecognitionSystem::clearRecognizers() {
    m_recognizers.clear();
}

void GestureRecognitionSystem::touchBegan(int touchId, float x, float y) {
    TouchPoint touch;
    touch.id = touchId;
    touch.x = x;
    touch.y = y;
    touch.prevX = x;
    touch.prevY = y;
    touch.startTime = std::chrono::steady_clock::now();
    touch.updateTime = touch.startTime;
    touch.active = true;
    
    m_touches.push_back(touch);
    processRecognizers();
}

void GestureRecognitionSystem::touchMoved(int touchId, float x, float y) {
    TouchPoint* touch = findTouch(touchId);
    if (touch) {
        touch->prevX = touch->x;
        touch->prevY = touch->y;
        touch->x = x;
        touch->y = y;
        touch->updateTime = std::chrono::steady_clock::now();
        processRecognizers();
    }
}

void GestureRecognitionSystem::touchEnded(int touchId, float x, float y) {
    TouchPoint* touch = findTouch(touchId);
    if (touch) {
        touch->x = x;
        touch->y = y;
        touch->active = false;
        touch->updateTime = std::chrono::steady_clock::now();
        processRecognizers();
        removeTouch(touchId);
    }
}

void GestureRecognitionSystem::touchCancelled(int touchId) {
    removeTouch(touchId);
    processRecognizers();
}

void GestureRecognitionSystem::mouseDown(int button, float x, float y) {
    if (button >= 0 && button < 3) {
        m_mouseDown[button] = true;
        m_mouseX = x;
        m_mouseY = y;
        touchBegan(button, x, y);
    }
}

void GestureRecognitionSystem::mouseMove(float x, float y) {
    m_mouseX = x;
    m_mouseY = y;
    for (int i = 0; i < 3; i++) {
        if (m_mouseDown[i]) {
            touchMoved(i, x, y);
        }
    }
}

void GestureRecognitionSystem::mouseUp(int button, float x, float y) {
    if (button >= 0 && button < 3 && m_mouseDown[button]) {
        m_mouseDown[button] = false;
        touchEnded(button, x, y);
    }
}

void GestureRecognitionSystem::update(float deltaTime) {
    processRecognizers();
}

void GestureRecognitionSystem::setGlobalConfig(const GestureConfig& config) {
    m_globalConfig = config;
    for (auto& recognizer : m_recognizers) {
        recognizer->setConfig(config);
    }
}

int GestureRecognitionSystem::getActiveTouchCount() const {
    int count = 0;
    for (const auto& touch : m_touches) {
        if (touch.active) count++;
    }
    return count;
}

TouchPoint* GestureRecognitionSystem::findTouch(int touchId) {
    for (auto& touch : m_touches) {
        if (touch.id == touchId) return &touch;
    }
    return nullptr;
}

void GestureRecognitionSystem::removeTouch(int touchId) {
    m_touches.erase(
        std::remove_if(m_touches.begin(), m_touches.end(),
            [touchId](const TouchPoint& t) { return t.id == touchId; }),
        m_touches.end()
    );
}

void GestureRecognitionSystem::processRecognizers() {
    for (auto& recognizer : m_recognizers) {
        recognizer->recognize(m_touches);
    }
}

void GestureRecognitionSystem::setupCommonGestures() {
    // Add basic gestures with default callbacks
    addTapGesture(nullptr, 1);
    addTapGesture(nullptr, 2);
    addLongPressGesture(nullptr);
    addSwipeGesture(nullptr);
    addPinchGesture(nullptr);
    addPanGesture(nullptr);
}

std::shared_ptr<TapRecognizer> GestureRecognitionSystem::addTapGesture(
    GestureCallback callback, int taps) {
    auto recognizer = std::make_shared<TapRecognizer>(taps);
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

std::shared_ptr<LongPressRecognizer> GestureRecognitionSystem::addLongPressGesture(
    GestureCallback callback) {
    auto recognizer = std::make_shared<LongPressRecognizer>();
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

std::shared_ptr<SwipeRecognizer> GestureRecognitionSystem::addSwipeGesture(
    GestureCallback callback) {
    auto recognizer = std::make_shared<SwipeRecognizer>();
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

std::shared_ptr<PinchRecognizer> GestureRecognitionSystem::addPinchGesture(
    GestureCallback callback) {
    auto recognizer = std::make_shared<PinchRecognizer>();
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

std::shared_ptr<RotationRecognizer> GestureRecognitionSystem::addRotationGesture(
    GestureCallback callback) {
    auto recognizer = std::make_shared<RotationRecognizer>();
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

std::shared_ptr<PanRecognizer> GestureRecognitionSystem::addPanGesture(
    GestureCallback callback) {
    auto recognizer = std::make_shared<PanRecognizer>();
    recognizer->setCallback(callback);
    addRecognizer(recognizer);
    return recognizer;
}

} // namespace Input
} // namespace JJM
