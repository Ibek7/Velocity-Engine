#include "ui/UIAnimations.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace UI {

// Easing implementations
float Easing::apply(float t, EasingType type) {
    t = std::clamp(t, 0.0f, 1.0f);
    
    switch (type) {
        case EasingType::Linear: return linear(t);
        case EasingType::QuadIn: return quadIn(t);
        case EasingType::QuadOut: return quadOut(t);
        case EasingType::QuadInOut: return quadInOut(t);
        case EasingType::CubicIn: return cubicIn(t);
        case EasingType::CubicOut: return cubicOut(t);
        case EasingType::CubicInOut: return cubicInOut(t);
        case EasingType::SineIn: return sineIn(t);
        case EasingType::SineOut: return sineOut(t);
        case EasingType::SineInOut: return sineInOut(t);
        case EasingType::BounceIn: return bounceIn(t);
        case EasingType::BounceOut: return bounceOut(t);
        case EasingType::ElasticOut: return elasticOut(t);
        default: return t;
    }
}

float Easing::linear(float t) { return t; }
float Easing::quadIn(float t) { return t * t; }
float Easing::quadOut(float t) { return t * (2.0f - t); }
float Easing::quadInOut(float t) {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}
float Easing::cubicIn(float t) { return t * t * t; }
float Easing::cubicOut(float t) { return (--t) * t * t + 1.0f; }
float Easing::cubicInOut(float t) {
    return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
}
float Easing::sineIn(float t) { return 1.0f - std::cos(t * M_PI * 0.5f); }
float Easing::sineOut(float t) { return std::sin(t * M_PI * 0.5f); }
float Easing::sineInOut(float t) { return -(std::cos(M_PI * t) - 1.0f) * 0.5f; }
float Easing::bounceOut(float t) {
    if (t < 1.0f / 2.75f) return 7.5625f * t * t;
    if (t < 2.0f / 2.75f) return 7.5625f * (t -= 1.5f / 2.75f) * t + 0.75f;
    if (t < 2.5f / 2.75f) return 7.5625f * (t -= 2.25f / 2.75f) * t + 0.9375f;
    return 7.5625f * (t -= 2.625f / 2.75f) * t + 0.984375f;
}
float Easing::bounceIn(float t) { return 1.0f - bounceOut(1.0f - t); }
float Easing::elasticOut(float t) {
    if (t == 0.0f || t == 1.0f) return t;
    return std::pow(2.0f, -10.0f * t) * std::sin((t - 0.075f) * (2.0f * M_PI) / 0.3f) + 1.0f;
}

// UIAnimation implementation
UIAnimation::UIAnimation(float duration)
    : duration(duration), elapsedTime(0.0f), delay(0.0f), delayTimer(0.0f),
      playing(false), complete(false), loop(false), easingType(EasingType::Linear) {}

UIAnimation::~UIAnimation() {}

void UIAnimation::update(float deltaTime) {
    if (!playing || complete) return;
    
    if (delayTimer < delay) {
        delayTimer += deltaTime;
        return;
    }
    
    elapsedTime += deltaTime;
    
    if (elapsedTime >= duration) {
        if (loop) {
            elapsedTime = 0.0f;
            delayTimer = 0.0f;
        } else {
            elapsedTime = duration;
            complete = true;
            playing = false;
        }
    }
    
    float progress = std::clamp(elapsedTime / duration, 0.0f, 1.0f);
    float easedProgress = Easing::apply(progress, easingType);
    applyAnimation(easedProgress);
    
    if (onUpdate) onUpdate(easedProgress);
    if (complete && onComplete) onComplete();
}

void UIAnimation::reset() {
    elapsedTime = 0.0f;
    delayTimer = 0.0f;
    complete = false;
}

bool UIAnimation::isComplete() const { return complete; }
bool UIAnimation::isPlaying() const { return playing; }

void UIAnimation::play() { playing = true; complete = false; }
void UIAnimation::pause() { playing = false; }
void UIAnimation::stop() { playing = false; complete = false; reset(); }
void UIAnimation::setLoop(bool l) { loop = l; }

void UIAnimation::setDuration(float d) { duration = d; }
float UIAnimation::getDuration() const { return duration; }

void UIAnimation::setDelay(float d) { delay = d; }
void UIAnimation::setEasing(EasingType easing) { easingType = easing; }

void UIAnimation::setOnComplete(std::function<void()> callback) { onComplete = callback; }
void UIAnimation::setOnUpdate(std::function<void(float)> callback) { onUpdate = callback; }

// FadeAnimation implementation
FadeAnimation::FadeAnimation(float duration, float from, float to)
    : UIAnimation(duration), target(nullptr), fromValue(from), toValue(to) {}

FadeAnimation::~FadeAnimation() {}

void FadeAnimation::setTarget(float* t) { target = t; }
void FadeAnimation::setRange(float from, float to) {
    fromValue = from;
    toValue = to;
}

void FadeAnimation::applyAnimation(float progress) {
    if (target) {
        *target = fromValue + (toValue - fromValue) * progress;
    }
}

// MoveAnimation implementation
MoveAnimation::MoveAnimation(float duration, float fx, float fy, float tx, float ty)
    : UIAnimation(duration), targetX(nullptr), targetY(nullptr),
      fromX(fx), fromY(fy), toX(tx), toY(ty) {}

MoveAnimation::~MoveAnimation() {}

void MoveAnimation::setTarget(float* tx, float* ty) {
    targetX = tx;
    targetY = ty;
}

void MoveAnimation::setFrom(float x, float y) { fromX = x; fromY = y; }
void MoveAnimation::setTo(float x, float y) { toX = x; toY = y; }

void MoveAnimation::applyAnimation(float progress) {
    if (targetX) *targetX = fromX + (toX - fromX) * progress;
    if (targetY) *targetY = fromY + (toY - fromY) * progress;
}

// ScaleAnimation implementation
ScaleAnimation::ScaleAnimation(float duration, float from, float to)
    : UIAnimation(duration), targetScaleX(nullptr), targetScaleY(nullptr),
      fromScaleX(from), fromScaleY(from), toScaleX(to), toScaleY(to) {}

ScaleAnimation::~ScaleAnimation() {}

void ScaleAnimation::setTarget(float* scaleX, float* scaleY) {
    targetScaleX = scaleX;
    targetScaleY = scaleY;
}

void ScaleAnimation::setRange(float from, float to) {
    fromScaleX = fromScaleY = from;
    toScaleX = toScaleY = to;
}

void ScaleAnimation::setIndependentScales(float fx, float fy, float tx, float ty) {
    fromScaleX = fx; fromScaleY = fy;
    toScaleX = tx; toScaleY = ty;
}

void ScaleAnimation::applyAnimation(float progress) {
    if (targetScaleX) *targetScaleX = fromScaleX + (toScaleX - fromScaleX) * progress;
    if (targetScaleY) *targetScaleY = fromScaleY + (toScaleY - fromScaleY) * progress;
}

// RotationAnimation implementation
RotationAnimation::RotationAnimation(float duration, float from, float to)
    : UIAnimation(duration), targetAngle(nullptr), fromAngle(from), toAngle(to) {}

RotationAnimation::~RotationAnimation() {}

void RotationAnimation::setTarget(float* angle) { targetAngle = angle; }
void RotationAnimation::setRange(float from, float to) {
    fromAngle = from;
    toAngle = to;
}

void RotationAnimation::applyAnimation(float progress) {
    if (targetAngle) {
        *targetAngle = fromAngle + (toAngle - fromAngle) * progress;
    }
}

// ColorAnimation implementation
ColorAnimation::ColorAnimation(float duration)
    : UIAnimation(duration),
      targetR(nullptr), targetG(nullptr), targetB(nullptr), targetA(nullptr),
      fromR(1), fromG(1), fromB(1), fromA(1),
      toR(1), toG(1), toB(1), toA(1) {}

ColorAnimation::~ColorAnimation() {}

void ColorAnimation::setTarget(float* r, float* g, float* b, float* a) {
    targetR = r; targetG = g; targetB = b; targetA = a;
}

void ColorAnimation::setFrom(float r, float g, float b, float a) {
    fromR = r; fromG = g; fromB = b; fromA = a;
}

void ColorAnimation::setTo(float r, float g, float b, float a) {
    toR = r; toG = g; toB = b; toA = a;
}

void ColorAnimation::applyAnimation(float progress) {
    if (targetR) *targetR = fromR + (toR - fromR) * progress;
    if (targetG) *targetG = fromG + (toG - fromG) * progress;
    if (targetB) *targetB = fromB + (toB - fromB) * progress;
    if (targetA) *targetA = fromA + (toA - fromA) * progress;
}

// SequenceAnimation implementation
SequenceAnimation::SequenceAnimation()
    : UIAnimation(0.0f), currentAnimationIndex(0) {}

SequenceAnimation::~SequenceAnimation() {}

void SequenceAnimation::addAnimation(std::unique_ptr<UIAnimation> animation) {
    duration += animation->getDuration();
    animations.push_back(std::move(animation));
}

void SequenceAnimation::update(float deltaTime) {
    if (!playing || complete || currentAnimationIndex >= animations.size()) return;
    
    auto& currentAnim = animations[currentAnimationIndex];
    currentAnim->update(deltaTime);
    
    if (currentAnim->isComplete()) {
        currentAnimationIndex++;
        if (currentAnimationIndex >= animations.size()) {
            complete = true;
            playing = false;
            if (onComplete) onComplete();
        }
    }
}

void SequenceAnimation::reset() {
    UIAnimation::reset();
    currentAnimationIndex = 0;
    for (auto& anim : animations) {
        anim->reset();
    }
}

void SequenceAnimation::applyAnimation(float progress) {
    (void)progress; // Handled in update
}

// ParallelAnimation implementation
ParallelAnimation::ParallelAnimation() : UIAnimation(0.0f) {}
ParallelAnimation::~ParallelAnimation() {}

void ParallelAnimation::addAnimation(std::unique_ptr<UIAnimation> animation) {
    duration = std::max(duration, animation->getDuration());
    animations.push_back(std::move(animation));
}

void ParallelAnimation::update(float deltaTime) {
    if (!playing || complete) return;
    
    bool allComplete = true;
    for (auto& anim : animations) {
        anim->update(deltaTime);
        if (!anim->isComplete()) allComplete = false;
    }
    
    if (allComplete) {
        complete = true;
        playing = false;
        if (onComplete) onComplete();
    }
}

void ParallelAnimation::reset() {
    UIAnimation::reset();
    for (auto& anim : animations) {
        anim->reset();
    }
}

void ParallelAnimation::applyAnimation(float progress) {
    (void)progress; // Handled in update
}

// SpringAnimation implementation
SpringAnimation::SpringAnimation(float duration)
    : UIAnimation(duration), target(nullptr), targetValue(0.0f),
      currentValue(0.0f), velocity(0.0f), stiffness(100.0f), damping(10.0f) {}

SpringAnimation::~SpringAnimation() {}

void SpringAnimation::setTarget(float* t) { target = t; }
void SpringAnimation::setTargetValue(float value) { targetValue = value; }
void SpringAnimation::setSpringParams(float s, float d) {
    stiffness = s;
    damping = d;
}

void SpringAnimation::applyAnimation(float progress) {
    if (!target) return;
    
    float force = (targetValue - currentValue) * stiffness;
    float dampingForce = velocity * damping;
    velocity += (force - dampingForce) * 0.016f;
    currentValue += velocity * 0.016f;
    
    *target = currentValue;
}

// ShakeAnimation implementation
ShakeAnimation::ShakeAnimation(float duration, float intensity)
    : UIAnimation(duration), targetX(nullptr), targetY(nullptr),
      baseX(0), baseY(0), intensity(intensity), frequency(10.0f), time(0.0f) {}

ShakeAnimation::~ShakeAnimation() {}

void ShakeAnimation::setTarget(float* x, float* y) {
    targetX = x;
    targetY = y;
    if (x) baseX = *x;
    if (y) baseY = *y;
}

void ShakeAnimation::setIntensity(float i) { intensity = i; }
void ShakeAnimation::setFrequency(float f) { frequency = f; }

void ShakeAnimation::applyAnimation(float progress) {
    time += 0.016f;
    float decay = 1.0f - progress;
    float offsetX = std::sin(time * frequency) * intensity * decay;
    float offsetY = std::cos(time * frequency * 1.3f) * intensity * decay;
    
    if (targetX) *targetX = baseX + offsetX;
    if (targetY) *targetY = baseY + offsetY;
}

// PulseAnimation implementation
PulseAnimation::PulseAnimation(float duration, float min, float max)
    : UIAnimation(duration), targetScaleX(nullptr), targetScaleY(nullptr),
      minScale(min), maxScale(max) {}

PulseAnimation::~PulseAnimation() {}

void PulseAnimation::setTarget(float* scaleX, float* scaleY) {
    targetScaleX = scaleX;
    targetScaleY = scaleY;
}

void PulseAnimation::setScaleRange(float min, float max) {
    minScale = min;
    maxScale = max;
}

void PulseAnimation::applyAnimation(float progress) {
    float scale = minScale + (maxScale - minScale) * std::abs(std::sin(progress * M_PI));
    if (targetScaleX) *targetScaleX = scale;
    if (targetScaleY) *targetScaleY = scale;
}

// AnimationCurve implementation
AnimationCurve::AnimationCurve() {}
AnimationCurve::~AnimationCurve() {}

void AnimationCurve::addKeyframe(float time, float value) {
    keyframes.push_back({time, value});
    std::sort(keyframes.begin(), keyframes.end(),
              [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
}

float AnimationCurve::evaluate(float time) const {
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes[0].value;
    
    if (time <= keyframes.front().time) return keyframes.front().value;
    if (time >= keyframes.back().time) return keyframes.back().value;
    
    for (size_t i = 0; i < keyframes.size() - 1; ++i) {
        if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
            float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
            return keyframes[i].value + (keyframes[i + 1].value - keyframes[i].value) * t;
        }
    }
    
    return keyframes.back().value;
}

void AnimationCurve::clear() { keyframes.clear(); }

// CurveAnimation implementation
CurveAnimation::CurveAnimation(float duration)
    : UIAnimation(duration), target(nullptr) {}

CurveAnimation::~CurveAnimation() {}

void CurveAnimation::setTarget(float* t) { target = t; }
void CurveAnimation::setCurve(const AnimationCurve& c) { curve = c; }
AnimationCurve& CurveAnimation::getCurve() { return curve; }

void CurveAnimation::applyAnimation(float progress) {
    if (target) {
        *target = curve.evaluate(progress);
    }
}

// AnimationManager implementation
AnimationManager::AnimationManager() {}
AnimationManager::~AnimationManager() {}

void AnimationManager::addAnimation(const std::string& name, std::unique_ptr<UIAnimation> animation) {
    animations[name] = std::move(animation);
}

void AnimationManager::removeAnimation(const std::string& name) {
    animations.erase(name);
}

void AnimationManager::playAnimation(const std::string& name) {
    auto it = animations.find(name);
    if (it != animations.end()) {
        it->second->play();
    }
}

void AnimationManager::stopAnimation(const std::string& name) {
    auto it = animations.find(name);
    if (it != animations.end()) {
        it->second->stop();
    }
}

void AnimationManager::stopAllAnimations() {
    for (auto& pair : animations) {
        pair.second->stop();
    }
}

void AnimationManager::update(float deltaTime) {
    for (auto& pair : animations) {
        pair.second->update(deltaTime);
    }
}

UIAnimation* AnimationManager::getAnimation(const std::string& name) {
    auto it = animations.find(name);
    return it != animations.end() ? it->second.get() : nullptr;
}

bool AnimationManager::hasAnimation(const std::string& name) const {
    return animations.find(name) != animations.end();
}

// AnimationBuilder implementation
AnimationBuilder::AnimationBuilder()
    : currentDelay(0.0f), currentEasing(EasingType::Linear),
      currentLoop(false) {}

AnimationBuilder::~AnimationBuilder() {}

AnimationBuilder& AnimationBuilder::fade(float duration, float from, float to) {
    auto anim = std::make_unique<FadeAnimation>(duration, from, to);
    anim->setDelay(currentDelay);
    anim->setEasing(currentEasing);
    anim->setLoop(currentLoop);
    if (currentCallback) anim->setOnComplete(currentCallback);
    animations.push_back(std::move(anim));
    return *this;
}

AnimationBuilder& AnimationBuilder::move(float duration, float fromX, float fromY, float toX, float toY) {
    auto anim = std::make_unique<MoveAnimation>(duration, fromX, fromY, toX, toY);
    anim->setDelay(currentDelay);
    anim->setEasing(currentEasing);
    anim->setLoop(currentLoop);
    if (currentCallback) anim->setOnComplete(currentCallback);
    animations.push_back(std::move(anim));
    return *this;
}

AnimationBuilder& AnimationBuilder::scale(float duration, float from, float to) {
    auto anim = std::make_unique<ScaleAnimation>(duration, from, to);
    anim->setDelay(currentDelay);
    anim->setEasing(currentEasing);
    anim->setLoop(currentLoop);
    if (currentCallback) anim->setOnComplete(currentCallback);
    animations.push_back(std::move(anim));
    return *this;
}

AnimationBuilder& AnimationBuilder::rotate(float duration, float fromAngle, float toAngle) {
    auto anim = std::make_unique<RotationAnimation>(duration, fromAngle, toAngle);
    anim->setDelay(currentDelay);
    anim->setEasing(currentEasing);
    anim->setLoop(currentLoop);
    if (currentCallback) anim->setOnComplete(currentCallback);
    animations.push_back(std::move(anim));
    return *this;
}

AnimationBuilder& AnimationBuilder::delay(float d) {
    currentDelay = d;
    return *this;
}

AnimationBuilder& AnimationBuilder::easing(EasingType easing) {
    currentEasing = easing;
    return *this;
}

AnimationBuilder& AnimationBuilder::loop(bool l) {
    currentLoop = l;
    return *this;
}

AnimationBuilder& AnimationBuilder::onComplete(std::function<void()> callback) {
    currentCallback = callback;
    return *this;
}

std::unique_ptr<UIAnimation> AnimationBuilder::build() {
    if (animations.empty()) return nullptr;
    if (animations.size() == 1) return std::move(animations[0]);
    return buildSequence();
}

std::unique_ptr<SequenceAnimation> AnimationBuilder::buildSequence() {
    auto seq = std::make_unique<SequenceAnimation>();
    for (auto& anim : animations) {
        seq->addAnimation(std::move(anim));
    }
    animations.clear();
    return seq;
}

std::unique_ptr<ParallelAnimation> AnimationBuilder::buildParallel() {
    auto parallel = std::make_unique<ParallelAnimation>();
    for (auto& anim : animations) {
        parallel->addAnimation(std::move(anim));
    }
    animations.clear();
    return parallel;
}

} // namespace UI
} // namespace JJM
