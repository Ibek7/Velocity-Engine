#include "animation/AnimationCurves.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Engine {

// AnimationCurve Implementation
AnimationCurve::AnimationCurve()
    : m_easingType(EasingType::Linear)
    , m_useEasing(false) {
}

AnimationCurve::AnimationCurve(EasingType easing)
    : m_easingType(easing)
    , m_useEasing(true) {
}

AnimationCurve::~AnimationCurve() {
}

void AnimationCurve::addKeyframe(const Keyframe& keyframe) {
    m_keyframes.push_back(keyframe);
    sortKeyframes();
}

void AnimationCurve::removeKeyframe(int index) {
    if (index >= 0 && index < static_cast<int>(m_keyframes.size())) {
        m_keyframes.erase(m_keyframes.begin() + index);
    }
}

AnimationCurve::Keyframe& AnimationCurve::getKeyframe(int index) {
    return m_keyframes[index];
}

float AnimationCurve::evaluate(float time) const {
    if (m_useEasing) {
        return EasingFunctions::getFunction(m_easingType)(time);
    }
    
    if (m_keyframes.empty()) {
        return 0.0f;
    }
    
    if (m_keyframes.size() == 1) {
        return m_keyframes[0].value;
    }
    
    // Clamp time
    if (time <= m_keyframes.front().time) {
        return m_keyframes.front().value;
    }
    if (time >= m_keyframes.back().time) {
        return m_keyframes.back().value;
    }
    
    // Find surrounding keyframes
    for (size_t i = 0; i < m_keyframes.size() - 1; ++i) {
        if (time >= m_keyframes[i].time && time <= m_keyframes[i + 1].time) {
            return hermiteInterpolate(time, m_keyframes[i], m_keyframes[i + 1]);
        }
    }
    
    return m_keyframes.back().value;
}

void AnimationCurve::setEasing(EasingType easing) {
    m_easingType = easing;
    m_useEasing = true;
}

void AnimationCurve::clear() {
    m_keyframes.clear();
}

void AnimationCurve::smoothTangents() {
    for (size_t i = 0; i < m_keyframes.size(); ++i) {
        if (i == 0) {
            if (m_keyframes.size() > 1) {
                float slope = (m_keyframes[1].value - m_keyframes[0].value) /
                            (m_keyframes[1].time - m_keyframes[0].time);
                m_keyframes[0].outTangent = slope;
            }
        } else if (i == m_keyframes.size() - 1) {
            float slope = (m_keyframes[i].value - m_keyframes[i - 1].value) /
                        (m_keyframes[i].time - m_keyframes[i - 1].time);
            m_keyframes[i].inTangent = slope;
        } else {
            float slope = (m_keyframes[i + 1].value - m_keyframes[i - 1].value) /
                        (m_keyframes[i + 1].time - m_keyframes[i - 1].time);
            m_keyframes[i].inTangent = slope;
            m_keyframes[i].outTangent = slope;
        }
    }
}

void AnimationCurve::linearizeTangents() {
    for (auto& kf : m_keyframes) {
        kf.inTangent = 0.0f;
        kf.outTangent = 0.0f;
    }
}

AnimationCurve AnimationCurve::constant(float value) {
    AnimationCurve curve;
    curve.addKeyframe(Keyframe(0.0f, value));
    curve.addKeyframe(Keyframe(1.0f, value));
    return curve;
}

AnimationCurve AnimationCurve::linear(float start, float end) {
    AnimationCurve curve;
    curve.addKeyframe(Keyframe(0.0f, start));
    curve.addKeyframe(Keyframe(1.0f, end));
    return curve;
}

void AnimationCurve::sortKeyframes() {
    std::sort(m_keyframes.begin(), m_keyframes.end(),
        [](const Keyframe& a, const Keyframe& b) {
            return a.time < b.time;
        });
}

float AnimationCurve::hermiteInterpolate(float t, const Keyframe& k1, const Keyframe& k2) const {
    float dt = k2.time - k1.time;
    float t_norm = (t - k1.time) / dt;
    
    float t2 = t_norm * t_norm;
    float t3 = t2 * t_norm;
    
    float h1 = 2 * t3 - 3 * t2 + 1;
    float h2 = -2 * t3 + 3 * t2;
    float h3 = t3 - 2 * t2 + t_norm;
    float h4 = t3 - t2;
    
    return h1 * k1.value + h2 * k2.value + h3 * k1.outTangent * dt + h4 * k2.inTangent * dt;
}

// EasingFunctions Implementation
float EasingFunctions::linear(float t) {
    return t;
}

float EasingFunctions::quadIn(float t) {
    return t * t;
}

float EasingFunctions::quadOut(float t) {
    return t * (2.0f - t);
}

float EasingFunctions::quadInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

float EasingFunctions::cubicIn(float t) {
    return t * t * t;
}

float EasingFunctions::cubicOut(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

float EasingFunctions::cubicInOut(float t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}

float EasingFunctions::quartIn(float t) {
    return t * t * t * t;
}

float EasingFunctions::quartOut(float t) {
    float f = t - 1.0f;
    return 1.0f - f * f * f * f;
}

float EasingFunctions::quartInOut(float t) {
    if (t < 0.5f) {
        return 8.0f * t * t * t * t;
    } else {
        float f = t - 1.0f;
        return 1.0f - 8.0f * f * f * f * f;
    }
}

float EasingFunctions::quintIn(float t) {
    return t * t * t * t * t;
}

float EasingFunctions::quintOut(float t) {
    float f = t - 1.0f;
    return f * f * f * f * f + 1.0f;
}

float EasingFunctions::quintInOut(float t) {
    if (t < 0.5f) {
        return 16.0f * t * t * t * t * t;
    } else {
        float f = (2.0f * t) - 2.0f;
        return 0.5f * f * f * f * f * f + 1.0f;
    }
}

float EasingFunctions::sineIn(float t) {
    return 1.0f - std::cos(t * M_PI / 2.0f);
}

float EasingFunctions::sineOut(float t) {
    return std::sin(t * M_PI / 2.0f);
}

float EasingFunctions::sineInOut(float t) {
    return 0.5f * (1.0f - std::cos(t * M_PI));
}

float EasingFunctions::expoIn(float t) {
    return t == 0.0f ? 0.0f : std::pow(2.0f, 10.0f * (t - 1.0f));
}

float EasingFunctions::expoOut(float t) {
    return t == 1.0f ? 1.0f : 1.0f - std::pow(2.0f, -10.0f * t);
}

float EasingFunctions::expoInOut(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    
    if (t < 0.5f) {
        return 0.5f * std::pow(2.0f, (20.0f * t) - 10.0f);
    } else {
        return 0.5f * (2.0f - std::pow(2.0f, -20.0f * t + 10.0f));
    }
}

float EasingFunctions::circIn(float t) {
    return 1.0f - std::sqrt(1.0f - t * t);
}

float EasingFunctions::circOut(float t) {
    return std::sqrt((2.0f - t) * t);
}

float EasingFunctions::circInOut(float t) {
    if (t < 0.5f) {
        return 0.5f * (1.0f - std::sqrt(1.0f - 4.0f * t * t));
    } else {
        return 0.5f * (std::sqrt(-((2.0f * t) - 3.0f) * ((2.0f * t) - 1.0f)) + 1.0f);
    }
}

float EasingFunctions::elasticIn(float t) {
    return std::sin(13.0f * M_PI / 2.0f * t) * std::pow(2.0f, 10.0f * (t - 1.0f));
}

float EasingFunctions::elasticOut(float t) {
    return std::sin(-13.0f * M_PI / 2.0f * (t + 1.0f)) * std::pow(2.0f, -10.0f * t) + 1.0f;
}

float EasingFunctions::elasticInOut(float t) {
    if (t < 0.5f) {
        return 0.5f * std::sin(13.0f * M_PI / 2.0f * (2.0f * t)) * std::pow(2.0f, 10.0f * ((2.0f * t) - 1.0f));
    } else {
        return 0.5f * (std::sin(-13.0f * M_PI / 2.0f * ((2.0f * t - 1.0f) + 1.0f)) * 
                      std::pow(2.0f, -10.0f * (2.0f * t - 1.0f)) + 2.0f);
    }
}

float EasingFunctions::backIn(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return c3 * t * t * t - c1 * t * t;
}

float EasingFunctions::backOut(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(t - 1.0f, 3.0f) + c1 * std::pow(t - 1.0f, 2.0f);
}

float EasingFunctions::backInOut(float t) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;
    
    return t < 0.5f
        ? (std::pow(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
        : (std::pow(2.0f * t - 2.0f, 2.0f) * ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

float EasingFunctions::bounceOut(float t) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;
    
    if (t < 1.0f / d1) {
        return n1 * t * t;
    } else if (t < 2.0f / d1) {
        t -= 1.5f / d1;
        return n1 * t * t + 0.75f;
    } else if (t < 2.5f / d1) {
        t -= 2.25f / d1;
        return n1 * t * t + 0.9375f;
    } else {
        t -= 2.625f / d1;
        return n1 * t * t + 0.984375f;
    }
}

float EasingFunctions::bounceIn(float t) {
    return 1.0f - bounceOut(1.0f - t);
}

float EasingFunctions::bounceInOut(float t) {
    return t < 0.5f
        ? (1.0f - bounceOut(1.0f - 2.0f * t)) / 2.0f
        : (1.0f + bounceOut(2.0f * t - 1.0f)) / 2.0f;
}

std::function<float(float)> EasingFunctions::getFunction(EasingType type) {
    switch (type) {
        case EasingType::Linear: return linear;
        case EasingType::QuadIn: return quadIn;
        case EasingType::QuadOut: return quadOut;
        case EasingType::QuadInOut: return quadInOut;
        case EasingType::CubicIn: return cubicIn;
        case EasingType::CubicOut: return cubicOut;
        case EasingType::CubicInOut: return cubicInOut;
        case EasingType::QuartIn: return quartIn;
        case EasingType::QuartOut: return quartOut;
        case EasingType::QuartInOut: return quartInOut;
        case EasingType::QuintIn: return quintIn;
        case EasingType::QuintOut: return quintOut;
        case EasingType::QuintInOut: return quintInOut;
        case EasingType::SineIn: return sineIn;
        case EasingType::SineOut: return sineOut;
        case EasingType::SineInOut: return sineInOut;
        case EasingType::ExpoIn: return expoIn;
        case EasingType::ExpoOut: return expoOut;
        case EasingType::ExpoInOut: return expoInOut;
        case EasingType::CircIn: return circIn;
        case EasingType::CircOut: return circOut;
        case EasingType::CircInOut: return circInOut;
        case EasingType::ElasticIn: return elasticIn;
        case EasingType::ElasticOut: return elasticOut;
        case EasingType::ElasticInOut: return elasticInOut;
        case EasingType::BackIn: return backIn;
        case EasingType::BackOut: return backOut;
        case EasingType::BackInOut: return backInOut;
        case EasingType::BounceIn: return bounceIn;
        case EasingType::BounceOut: return bounceOut;
        case EasingType::BounceInOut: return bounceInOut;
        default: return linear;
    }
}

// CurveInterpolator Implementation
float CurveInterpolator::interpolate(float from, float to, float t, const AnimationCurve& curve) {
    float curveValue = curve.evaluate(t);
    return from + (to - from) * curveValue;
}

float CurveInterpolator::interpolate(float from, float to, float t, EasingType easing) {
    float easedT = EasingFunctions::getFunction(easing)(t);
    return from + (to - from) * easedT;
}

void CurveInterpolator::interpolateArray(const float* from, const float* to, float t,
                                        const AnimationCurve& curve, float* out, int count) {
    float curveValue = curve.evaluate(t);
    for (int i = 0; i < count; ++i) {
        out[i] = from[i] + (to[i] - from[i]) * curveValue;
    }
}

// SpringInterpolator Implementation
SpringInterpolator::SpringInterpolator()
    : m_current(0.0f)
    , m_velocity(0.0f)
    , m_target(0.0f)
    , m_stiffness(100.0f)
    , m_damping(10.0f) {
}

void SpringInterpolator::setParameters(float stiffness, float damping) {
    m_stiffness = stiffness;
    m_damping = damping;
}

void SpringInterpolator::setTarget(float target) {
    m_target = target;
}

void SpringInterpolator::update(float deltaTime) {
    float force = m_stiffness * (m_target - m_current);
    float damping = m_damping * m_velocity;
    float acceleration = force - damping;
    
    m_velocity += acceleration * deltaTime;
    m_current += m_velocity * deltaTime;
}

bool SpringInterpolator::isSettled(float threshold) const {
    return std::abs(m_target - m_current) < threshold && std::abs(m_velocity) < threshold;
}

void SpringInterpolator::reset(float value) {
    m_current = value;
    m_velocity = 0.0f;
    m_target = value;
}

// SmoothDamp Implementation
float SmoothDamp::smoothDamp(float current, float target, float& currentVelocity,
                            float smoothTime, float maxSpeed, float deltaTime) {
    smoothTime = std::max(0.0001f, smoothTime);
    float omega = 2.0f / smoothTime;
    float x = omega * deltaTime;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    
    float change = current - target;
    float originalTo = target;
    
    float maxChange = maxSpeed * smoothTime;
    change = std::max(-maxChange, std::min(change, maxChange));
    target = current - change;
    
    float temp = (currentVelocity + omega * change) * deltaTime;
    currentVelocity = (currentVelocity - omega * temp) * exp;
    float output = target + (change + temp) * exp;
    
    if ((originalTo - current > 0.0f) == (output > originalTo)) {
        output = originalTo;
        currentVelocity = (output - originalTo) / deltaTime;
    }
    
    return output;
}

float SmoothDamp::smoothDamp(float current, float target, float& currentVelocity,
                            float smoothTime, float deltaTime) {
    return smoothDamp(current, target, currentVelocity, smoothTime, 
                     std::numeric_limits<float>::infinity(), deltaTime);
}

} // namespace Engine
