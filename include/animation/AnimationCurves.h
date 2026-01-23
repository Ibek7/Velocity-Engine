#pragma once

#include <vector>
#include <functional>

/**
 * @file AnimationCurves.h
 * @brief Animation curve utilities and interpolation functions
 * 
 * Provides various curve types and easing functions for smooth
 * animation transitions and procedural motion.
 */

namespace Engine {

/**
 * @enum EasingType
 * @brief Standard easing function types
 */
enum class EasingType {
    Linear,
    QuadIn, QuadOut, QuadInOut,
    CubicIn, CubicOut, CubicInOut,
    QuartIn, QuartOut, QuartInOut,
    QuintIn, QuintOut, QuintInOut,
    SineIn, SineOut, SineInOut,
    ExpoIn, ExpoOut, ExpoInOut,
    CircIn, CircOut, CircInOut,
    ElasticIn, ElasticOut, ElasticInOut,
    BackIn, BackOut, BackInOut,
    BounceIn, BounceOut, BounceInOut
};

/**
 * @class AnimationCurve
 * @brief Represents a customizable animation curve
 */
class AnimationCurve {
public:
    /**
     * @struct Keyframe
     * @brief A single point on the animation curve
     */
    struct Keyframe {
        float time;           ///< Time value (0-1)
        float value;          ///< Output value
        float inTangent;      ///< Incoming tangent for smooth interpolation
        float outTangent;     ///< Outgoing tangent for smooth interpolation
        
        Keyframe(float t = 0.0f, float v = 0.0f, float inT = 0.0f, float outT = 0.0f)
            : time(t), value(v), inTangent(inT), outTangent(outT) {}
    };
    
    AnimationCurve();
    AnimationCurve(EasingType easing);
    ~AnimationCurve();
    
    /**
     * @brief Add a keyframe to the curve
     * @param keyframe Keyframe to add
     */
    void addKeyframe(const Keyframe& keyframe);
    
    /**
     * @brief Remove keyframe at index
     * @param index Keyframe index
     */
    void removeKeyframe(int index);
    
    /**
     * @brief Get keyframe at index
     * @param index Keyframe index
     * @return Keyframe reference
     */
    Keyframe& getKeyframe(int index);
    
    /**
     * @brief Get number of keyframes
     * @return Keyframe count
     */
    int getKeyframeCount() const { return static_cast<int>(m_keyframes.size()); }
    
    /**
     * @brief Evaluate curve at given time
     * @param time Time value (0-1)
     * @return Interpolated value
     */
    float evaluate(float time) const;
    
    /**
     * @brief Set curve to use a preset easing function
     * @param easing Easing type
     */
    void setEasing(EasingType easing);
    
    /**
     * @brief Clear all keyframes
     */
    void clear();
    
    /**
     * @brief Smooth tangents automatically
     */
    void smoothTangents();
    
    /**
     * @brief Set all tangents to linear
     */
    void linearizeTangents();
    
    /**
     * @brief Create a constant curve
     * @param value Constant value
     * @return Animation curve
     */
    static AnimationCurve constant(float value);
    
    /**
     * @brief Create a linear curve from start to end
     * @param start Start value
     * @param end End value
     * @return Animation curve
     */
    static AnimationCurve linear(float start, float end);

private:
    std::vector<Keyframe> m_keyframes;
    EasingType m_easingType;
    bool m_useEasing;
    
    void sortKeyframes();
    float hermiteInterpolate(float t, const Keyframe& k1, const Keyframe& k2) const;
};

/**
 * @class EasingFunctions
 * @brief Static easing function utilities
 */
class EasingFunctions {
public:
    // Linear
    static float linear(float t);
    
    // Quadratic
    static float quadIn(float t);
    static float quadOut(float t);
    static float quadInOut(float t);
    
    // Cubic
    static float cubicIn(float t);
    static float cubicOut(float t);
    static float cubicInOut(float t);
    
    // Quartic
    static float quartIn(float t);
    static float quartOut(float t);
    static float quartInOut(float t);
    
    // Quintic
    static float quintIn(float t);
    static float quintOut(float t);
    static float quintInOut(float t);
    
    // Sine
    static float sineIn(float t);
    static float sineOut(float t);
    static float sineInOut(float t);
    
    // Exponential
    static float expoIn(float t);
    static float expoOut(float t);
    static float expoInOut(float t);
    
    // Circular
    static float circIn(float t);
    static float circOut(float t);
    static float circInOut(float t);
    
    // Elastic
    static float elasticIn(float t);
    static float elasticOut(float t);
    static float elasticInOut(float t);
    
    // Back
    static float backIn(float t);
    static float backOut(float t);
    static float backInOut(float t);
    
    // Bounce
    static float bounceIn(float t);
    static float bounceOut(float t);
    static float bounceInOut(float t);
    
    /**
     * @brief Get easing function by type
     * @param type Easing type
     * @return Function pointer
     */
    static std::function<float(float)> getFunction(EasingType type);
};

/**
 * @class CurveInterpolator
 * @brief Utility for interpolating values along curves
 */
class CurveInterpolator {
public:
    /**
     * @brief Interpolate between two values using a curve
     * @param from Start value
     * @param to End value
     * @param t Interpolation factor (0-1)
     * @param curve Animation curve
     * @return Interpolated value
     */
    static float interpolate(float from, float to, float t, const AnimationCurve& curve);
    
    /**
     * @brief Interpolate between two values using easing
     * @param from Start value
     * @param to End value
     * @param t Interpolation factor (0-1)
     * @param easing Easing type
     * @return Interpolated value
     */
    static float interpolate(float from, float to, float t, EasingType easing);
    
    /**
     * @brief Interpolate a vector of values
     * @param from Start values
     * @param to End values
     * @param t Interpolation factor (0-1)
     * @param curve Animation curve
     * @param out Output values
     */
    static void interpolateArray(const float* from, const float* to, float t,
                                 const AnimationCurve& curve, float* out, int count);
};

/**
 * @class SpringInterpolator
 * @brief Physics-based spring interpolation for smooth motion
 */
class SpringInterpolator {
public:
    SpringInterpolator();
    
    /**
     * @brief Set spring parameters
     * @param stiffness Spring stiffness (higher = tighter)
     * @param damping Damping coefficient (higher = less oscillation)
     */
    void setParameters(float stiffness, float damping);
    
    /**
     * @brief Set target value
     * @param target Target value to reach
     */
    void setTarget(float target);
    
    /**
     * @brief Get current value
     * @return Current interpolated value
     */
    float getValue() const { return m_current; }
    
    /**
     * @brief Update spring simulation
     * @param deltaTime Time step
     */
    void update(float deltaTime);
    
    /**
     * @brief Check if spring has settled
     * @param threshold Threshold for considering settled
     * @return True if settled
     */
    bool isSettled(float threshold = 0.001f) const;
    
    /**
     * @brief Reset to a specific value
     * @param value Initial value
     */
    void reset(float value);

private:
    float m_current;
    float m_velocity;
    float m_target;
    float m_stiffness;
    float m_damping;
};

/**
 * @class SmoothDamp
 * @brief Smoothly interpolate towards a target value
 */
class SmoothDamp {
public:
    /**
     * @brief Smooth damp interpolation
     * @param current Current value
     * @param target Target value
     * @param currentVelocity Current velocity (modified)
     * @param smoothTime Approximate time to reach target
     * @param maxSpeed Maximum speed (0 = unlimited)
     * @param deltaTime Time step
     * @return New interpolated value
     */
    static float smoothDamp(float current, float target, float& currentVelocity,
                           float smoothTime, float maxSpeed, float deltaTime);
    
    /**
     * @brief Smooth damp with default max speed
     * @param current Current value
     * @param target Target value
     * @param currentVelocity Current velocity (modified)
     * @param smoothTime Approximate time to reach target
     * @param deltaTime Time step
     * @return New interpolated value
     */
    static float smoothDamp(float current, float target, float& currentVelocity,
                           float smoothTime, float deltaTime);
};

} // namespace Engine
