#ifndef ANIMATION_H
#define ANIMATION_H

#include "math/Vector2D.h"
#include "graphics/Texture.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Animation {

// Sprite frame for sprite sheet animation
struct SpriteFrame {
    int x, y, width, height;
    float duration;
    
    SpriteFrame(int x = 0, int y = 0, int w = 0, int h = 0, float dur = 0.1f)
        : x(x), y(y), width(w), height(h), duration(dur) {}
};

// Animation clip containing frames
class AnimationClip {
private:
    std::vector<SpriteFrame> frames;
    std::string name;
    bool looping;
    
public:
    AnimationClip(const std::string& name = "", bool loop = true);
    
    void addFrame(const SpriteFrame& frame);
    void addFrame(int x, int y, int w, int h, float duration = 0.1f);
    
    const SpriteFrame& getFrame(int index) const;
    int getFrameCount() const { return static_cast<int>(frames.size()); }
    bool isLooping() const { return looping; }
    const std::string& getName() const { return name; }
};

// Animator for playing animation clips
class Animator {
private:
    std::unordered_map<std::string, AnimationClip> clips;
    std::string currentClipName;
    int currentFrame;
    float frameTimer;
    bool playing;
    bool finished;
    
    std::function<void()> onAnimationComplete;
    
public:
    Animator();
    
    void addClip(const AnimationClip& clip);
    void play(const std::string& clipName, bool restart = false);
    void pause();
    void resume();
    void stop();
    void reset();
    
    void update(float deltaTime);
    
    const SpriteFrame* getCurrentFrame() const;
    int getCurrentFrameIndex() const { return currentFrame; }
    const std::string& getCurrentClipName() const { return currentClipName; }
    
    bool isPlaying() const { return playing; }
    bool isFinished() const { return finished; }
    
    void setOnAnimationComplete(const std::function<void()>& callback) {
        onAnimationComplete = callback;
    }
};

// Tween functions for interpolation
enum class EaseType {
    Linear,
    QuadIn, QuadOut, QuadInOut,
    CubicIn, CubicOut, CubicInOut,
    SineIn, SineOut, SineInOut,
    ExpoIn, ExpoOut, ExpoInOut,
    CircIn, CircOut, CircInOut,
    ElasticIn, ElasticOut, ElasticInOut,
    BackIn, BackOut, BackInOut,
    BounceIn, BounceOut, BounceInOut
};

// Tween class for value interpolation
template<typename T>
class Tween {
private:
    T startValue;
    T endValue;
    T currentValue;
    float duration;
    float elapsed;
    EaseType easeType;
    bool active;
    bool complete;
    
    std::function<void(const T&)> onUpdate;
    std::function<void()> onComplete;
    
public:
    Tween(const T& start, const T& end, float dur, EaseType ease = EaseType::Linear)
        : startValue(start), endValue(end), currentValue(start),
          duration(dur), elapsed(0), easeType(ease),
          active(false), complete(false) {}
    
    void start() {
        active = true;
        complete = false;
        elapsed = 0;
        currentValue = startValue;
    }
    
    void update(float deltaTime) {
        if (!active || complete) return;
        
        elapsed += deltaTime;
        if (elapsed >= duration) {
            elapsed = duration;
            complete = true;
            active = false;
        }
        
        float t = elapsed / duration;
        float easedT = applyEasing(t);
        currentValue = interpolate(startValue, endValue, easedT);
        
        if (onUpdate) {
            onUpdate(currentValue);
        }
        
        if (complete && onComplete) {
            onComplete();
        }
    }
    
    void setOnUpdate(const std::function<void(const T&)>& callback) {
        onUpdate = callback;
    }
    
    void setOnComplete(const std::function<void()>& callback) {
        onComplete = callback;
    }
    
    const T& getValue() const { return currentValue; }
    bool isActive() const { return active; }
    bool isComplete() const { return complete; }
    
private:
    float applyEasing(float t);
    T interpolate(const T& a, const T& b, float t);
};

// Easing functions
class Ease {
public:
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
    static float expoIn(float t);
    static float expoOut(float t);
    static float expoInOut(float t);
    static float circIn(float t);
    static float circOut(float t);
    static float circInOut(float t);
    static float elasticIn(float t);
    static float elasticOut(float t);
    static float elasticInOut(float t);
    static float backIn(float t);
    static float backOut(float t);
    static float backInOut(float t);
    static float bounceIn(float t);
    static float bounceOut(float t);
    static float bounceInOut(float t);
};

} // namespace Animation
} // namespace JJM

#endif // ANIMATION_H
