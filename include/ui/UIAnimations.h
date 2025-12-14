#ifndef JJM_UI_ANIMATIONS_H
#define JJM_UI_ANIMATIONS_H

#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace JJM {
namespace UI {

/**
 * @brief Easing functions for smooth animations
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
 * @brief Easing function calculator
 */
class Easing {
public:
    static float apply(float t, EasingType type);

private:
    static float linear(float t);
    static float quadIn(float t);
    static float quadOut(float t);
    static float quadInOut(float t);
    static float cubicIn(float t);
    static float cubicOut(float t);
    static float cubicInOut(float t);
    static float sineIn(float t);
    static float sineOut(float t);
    static float sineInOut(float t);
    static float bounceOut(float t);
    static float bounceIn(float t);
    static float elasticOut(float t);
};

/**
 * @brief Base class for UI animations
 */
class UIAnimation {
public:
    UIAnimation(float duration);
    virtual ~UIAnimation();

    virtual void update(float deltaTime);
    virtual void reset();
    
    bool isComplete() const;
    bool isPlaying() const;
    
    void play();
    void pause();
    void stop();
    void setLoop(bool loop);
    
    void setDuration(float duration);
    float getDuration() const;
    
    void setDelay(float delay);
    void setEasing(EasingType easing);
    
    void setOnComplete(std::function<void()> callback);
    void setOnUpdate(std::function<void(float)> callback);

protected:
    virtual void applyAnimation(float progress) = 0;
    
    float duration;
    float elapsedTime;
    float delay;
    float delayTimer;
    bool playing;
    bool complete;
    bool loop;
    EasingType easingType;
    
    std::function<void()> onComplete;
    std::function<void(float)> onUpdate;
};

/**
 * @brief Fade animation for opacity
 */
class FadeAnimation : public UIAnimation {
public:
    FadeAnimation(float duration, float from, float to);
    ~FadeAnimation() override;

    void setTarget(float* target);
    void setRange(float from, float to);

protected:
    void applyAnimation(float progress) override;

private:
    float* target;
    float fromValue;
    float toValue;
};

/**
 * @brief Move animation for position
 */
class MoveAnimation : public UIAnimation {
public:
    MoveAnimation(float duration, float fromX, float fromY, float toX, float toY);
    ~MoveAnimation() override;

    void setTarget(float* targetX, float* targetY);
    void setFrom(float x, float y);
    void setTo(float x, float y);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetX;
    float* targetY;
    float fromX, fromY;
    float toX, toY;
};

/**
 * @brief Scale animation for size
 */
class ScaleAnimation : public UIAnimation {
public:
    ScaleAnimation(float duration, float fromScale, float toScale);
    ~ScaleAnimation() override;

    void setTarget(float* scaleX, float* scaleY);
    void setRange(float from, float to);
    void setIndependentScales(float fromX, float fromY, float toX, float toY);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetScaleX;
    float* targetScaleY;
    float fromScaleX, fromScaleY;
    float toScaleX, toScaleY;
};

/**
 * @brief Rotation animation
 */
class RotationAnimation : public UIAnimation {
public:
    RotationAnimation(float duration, float fromAngle, float toAngle);
    ~RotationAnimation() override;

    void setTarget(float* angle);
    void setRange(float from, float to);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetAngle;
    float fromAngle;
    float toAngle;
};

/**
 * @brief Color animation for tinting
 */
class ColorAnimation : public UIAnimation {
public:
    ColorAnimation(float duration);
    ~ColorAnimation() override;

    void setTarget(float* r, float* g, float* b, float* a);
    void setFrom(float r, float g, float b, float a);
    void setTo(float r, float g, float b, float a);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetR;
    float* targetG;
    float* targetB;
    float* targetA;
    float fromR, fromG, fromB, fromA;
    float toR, toG, toB, toA;
};

/**
 * @brief Sequence animation for chaining animations
 */
class SequenceAnimation : public UIAnimation {
public:
    SequenceAnimation();
    ~SequenceAnimation() override;

    void addAnimation(std::unique_ptr<UIAnimation> animation);
    void update(float deltaTime) override;
    void reset() override;

protected:
    void applyAnimation(float progress) override;

private:
    std::vector<std::unique_ptr<UIAnimation>> animations;
    size_t currentAnimationIndex;
};

/**
 * @brief Parallel animation for simultaneous animations
 */
class ParallelAnimation : public UIAnimation {
public:
    ParallelAnimation();
    ~ParallelAnimation() override;

    void addAnimation(std::unique_ptr<UIAnimation> animation);
    void update(float deltaTime) override;
    void reset() override;

protected:
    void applyAnimation(float progress) override;

private:
    std::vector<std::unique_ptr<UIAnimation>> animations;
};

/**
 * @brief Spring animation for physics-based motion
 */
class SpringAnimation : public UIAnimation {
public:
    SpringAnimation(float duration);
    ~SpringAnimation() override;

    void setTarget(float* target);
    void setTargetValue(float value);
    void setSpringParams(float stiffness, float damping);

protected:
    void applyAnimation(float progress) override;

private:
    float* target;
    float targetValue;
    float currentValue;
    float velocity;
    float stiffness;
    float damping;
};

/**
 * @brief Shake animation for impact effects
 */
class ShakeAnimation : public UIAnimation {
public:
    ShakeAnimation(float duration, float intensity);
    ~ShakeAnimation() override;

    void setTarget(float* x, float* y);
    void setIntensity(float intensity);
    void setFrequency(float frequency);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetX;
    float* targetY;
    float baseX, baseY;
    float intensity;
    float frequency;
    float time;
};

/**
 * @brief Pulse animation for scaling effect
 */
class PulseAnimation : public UIAnimation {
public:
    PulseAnimation(float duration, float minScale, float maxScale);
    ~PulseAnimation() override;

    void setTarget(float* scaleX, float* scaleY);
    void setScaleRange(float min, float max);

protected:
    void applyAnimation(float progress) override;

private:
    float* targetScaleX;
    float* targetScaleY;
    float minScale;
    float maxScale;
};

/**
 * @brief Animation curve for custom interpolation
 */
class AnimationCurve {
public:
    AnimationCurve();
    ~AnimationCurve();

    void addKeyframe(float time, float value);
    float evaluate(float time) const;
    void clear();

private:
    struct Keyframe {
        float time;
        float value;
    };
    
    std::vector<Keyframe> keyframes;
};

/**
 * @brief Custom curve animation
 */
class CurveAnimation : public UIAnimation {
public:
    CurveAnimation(float duration);
    ~CurveAnimation() override;

    void setTarget(float* target);
    void setCurve(const AnimationCurve& curve);
    AnimationCurve& getCurve();

protected:
    void applyAnimation(float progress) override;

private:
    float* target;
    AnimationCurve curve;
};

/**
 * @brief Animation manager for controlling multiple animations
 */
class AnimationManager {
public:
    AnimationManager();
    ~AnimationManager();

    void addAnimation(const std::string& name, std::unique_ptr<UIAnimation> animation);
    void removeAnimation(const std::string& name);
    void playAnimation(const std::string& name);
    void stopAnimation(const std::string& name);
    void stopAllAnimations();
    
    void update(float deltaTime);
    
    UIAnimation* getAnimation(const std::string& name);
    bool hasAnimation(const std::string& name) const;

private:
    std::unordered_map<std::string, std::unique_ptr<UIAnimation>> animations;
};

/**
 * @brief Animation builder for fluent API
 */
class AnimationBuilder {
public:
    AnimationBuilder();
    ~AnimationBuilder();

    AnimationBuilder& fade(float duration, float from, float to);
    AnimationBuilder& move(float duration, float fromX, float fromY, float toX, float toY);
    AnimationBuilder& scale(float duration, float from, float to);
    AnimationBuilder& rotate(float duration, float fromAngle, float toAngle);
    
    AnimationBuilder& delay(float delay);
    AnimationBuilder& easing(EasingType easing);
    AnimationBuilder& loop(bool loop);
    AnimationBuilder& onComplete(std::function<void()> callback);
    
    std::unique_ptr<UIAnimation> build();
    std::unique_ptr<SequenceAnimation> buildSequence();
    std::unique_ptr<ParallelAnimation> buildParallel();

private:
    std::vector<std::unique_ptr<UIAnimation>> animations;
    float currentDelay;
    EasingType currentEasing;
    bool currentLoop;
    std::function<void()> currentCallback;
};

} // namespace UI
} // namespace JJM

#endif // JJM_UI_ANIMATIONS_H
