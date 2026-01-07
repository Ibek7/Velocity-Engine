#ifndef TRANSITION_H
#define TRANSITION_H

#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <functional>
#include <SDL.h>

namespace JJM {
namespace Graphics {

enum class TransitionType {
    FADE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    WIPE_LEFT,
    WIPE_RIGHT,
    CIRCLE_OPEN,
    CIRCLE_CLOSE,
    PIXELATE
};

class Transition {
protected:
    float duration;
    float elapsed;
    bool finished;
    TransitionType type;
    
public:
    using CompleteCallback = std::function<void()>;
    CompleteCallback onComplete;
    
    Transition(TransitionType type, float duration);
    virtual ~Transition() = default;
    
    virtual void update(float deltaTime);
    virtual void render(Renderer* renderer) = 0;
    
    void reset();
    bool isFinished() const { return finished; }
    float getProgress() const;
    
    TransitionType getType() const { return type; }
};

class FadeTransition : public Transition {
private:
    Color color;
    bool fadeIn;
    
public:
    FadeTransition(float duration, const Color& color = Color::Black(), bool fadeIn = false);
    
    void render(Renderer* renderer) override;
};

class SlideTransition : public Transition {
private:
    Math::Vector2D direction;
    int screenWidth;
    int screenHeight;
    
public:
    SlideTransition(TransitionType type, float duration, int screenW, int screenH);
    
    void render(Renderer* renderer) override;
};

class WipeTransition : public Transition {
private:
    bool leftToRight;
    int screenWidth;
    int screenHeight;
    
public:
    WipeTransition(bool leftToRight, float duration, int screenW, int screenH);
    
    void render(Renderer* renderer) override;
};

class CircleTransition : public Transition {
private:
    bool opening;
    Math::Vector2D center;
    float maxRadius;
    
public:
    CircleTransition(bool opening, float duration, int screenW, int screenH);
    CircleTransition(bool opening, float duration, const Math::Vector2D& center, float maxRadius);
    
    void render(Renderer* renderer) override;
    
private:
    void renderCircle(Renderer* renderer, const Math::Vector2D& center, float radius);
};

class PixelateTransition : public Transition {
private:
    int screenWidth;
    int screenHeight;
    int maxPixelSize;
    
public:
    PixelateTransition(float duration, int screenW, int screenH, int maxPixelSize = 32);
    
    void render(Renderer* renderer) override;
};

// =============================================================================
// EASING FUNCTIONS
// =============================================================================

enum class EaseType {
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

class EasingFunctions {
public:
    static float ease(float t, EaseType type);
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
    static float elasticIn(float t);
    static float elasticOut(float t);
    static float backIn(float t);
    static float backOut(float t);
    static float bounceOut(float t);
};

// =============================================================================
// SHADER-BASED TRANSITIONS
// =============================================================================

class ShaderTransition : public Transition {
protected:
    SDL_Texture* renderTexture;
    std::string shaderName;
    float customParam1{0.0f};
    float customParam2{0.0f};
    
public:
    ShaderTransition(TransitionType type, float duration, const std::string& shader);
    virtual ~ShaderTransition();
    
    void setCustomParam(float param1, float param2 = 0.0f);
    void render(Renderer* renderer) override;
};

class DissolveTransition : public ShaderTransition {
private:
    SDL_Texture* noiseTexture;
    
public:
    DissolveTransition(float duration, SDL_Texture* noise);
    void render(Renderer* renderer) override;
};

class RippleTransition : public ShaderTransition {
private:
    Math::Vector2D center;
    float frequency{10.0f};
    float amplitude{0.05f};
    
public:
    RippleTransition(float duration, const Math::Vector2D& center);
    void setRippleParams(float freq, float amp);
    void render(Renderer* renderer) override;
};

class PageTurnTransition : public ShaderTransition {
private:
    bool turnRight{true};
    
public:
    PageTurnTransition(float duration, bool right = true);
    void render(Renderer* renderer) override;
};

// =============================================================================
// TRANSITION CHAINING
// =============================================================================

class TransitionSequence {
private:
    std::vector<Transition*> transitions;
    size_t currentIndex{0};
    bool loop{false};
    
public:
    TransitionSequence(bool looping = false);
    ~TransitionSequence();
    
    void addTransition(Transition* transition);
    void clearTransitions();
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    bool isFinished() const;
    void reset();
    float getTotalDuration() const;
};

class ParallelTransition {
private:
    std::vector<Transition*> transitions;
    
public:
    ParallelTransition();
    ~ParallelTransition();
    
    void addTransition(Transition* transition);
    void clearTransitions();
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    bool isFinished() const;
    void reset();
};

// =============================================================================
// ADVANCED TRANSITIONS
// =============================================================================

class MosaicTransition : public Transition {
private:
    int gridSize{10};
    int screenWidth, screenHeight;
    std::vector<float> cellProgresses;
    
public:
    MosaicTransition(float duration, int screenW, int screenH, int size = 10);
    void render(Renderer* renderer) override;
};

class BlindsTransition : public Transition {
private:
    int blindCount{10};
    bool horizontal{false};
    int screenWidth, screenHeight;
    
public:
    BlindsTransition(float duration, int screenW, int screenH, int count = 10, bool horiz = false);
    void render(Renderer* renderer) override;
};

class IrisTransition : public Transition {
private:
    Math::Vector2D center;
    bool closing{true};
    float maxRadius;
    int sides{16};
    
public:
    IrisTransition(float duration, bool close, int screenW, int screenH, int polygonSides = 16);
    void setCenter(const Math::Vector2D& pos);
    void render(Renderer* renderer) override;
};

class ZoomTransition : public Transition {
private:
    Math::Vector2D center;
    bool zoomIn{true};
    float startScale{0.0f};
    float endScale{1.0f};
    int screenWidth, screenHeight;
    
public:
    ZoomTransition(float duration, bool in, int screenW, int screenH);
    void setScaleRange(float start, float end);
    void setCenter(const Math::Vector2D& pos);
    void render(Renderer* renderer) override;
};

class DoorTransition : public Transition {
private:
    bool opening{false};
    int screenWidth, screenHeight;
    
public:
    DoorTransition(float duration, bool open, int screenW, int screenH);
    void render(Renderer* renderer) override;
};

class TransitionManager {
private:
    Transition* currentTransition;
    TransitionSequence* currentSequence{nullptr};
    ParallelTransition* currentParallel{nullptr};
    EaseType defaultEasing{EaseType::QuadInOut};
    
    static TransitionManager* instance;
    TransitionManager();
    
public:
    static TransitionManager* getInstance();
    ~TransitionManager();
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    void startTransition(Transition* transition);
    void startSequence(TransitionSequence* sequence);
    void startParallel(ParallelTransition* parallel);
    void clearTransition();
    
    bool isTransitioning() const;
    Transition* getCurrentTransition() { return currentTransition; }
    
    // Easing control
    void setDefaultEasing(EaseType type) { defaultEasing = type; }
    EaseType getDefaultEasing() const { return defaultEasing; }
    
    // Convenience methods
    void fadeOut(float duration, const Color& color = Color::Black(), EaseType easing = EaseType::Linear);
    void fadeIn(float duration, const Color& color = Color::Black(), EaseType easing = EaseType::Linear);
    void slideLeft(float duration, int screenW, int screenH);
    void slideRight(float duration, int screenW, int screenH);
    void circleClose(float duration, int screenW, int screenH);
    void circleOpen(float duration, int screenW, int screenH);
    void dissolve(float duration, SDL_Texture* noise);
    void ripple(float duration, const Math::Vector2D& center);
    void pageTurn(float duration, bool right = true);
    void mosaic(float duration, int screenW, int screenH, int gridSize = 10);
    void blinds(float duration, int screenW, int screenH, int count = 10, bool horizontal = false);
    void iris(float duration, bool close, int screenW, int screenH, int sides = 16);
    void zoom(float duration, bool in, int screenW, int screenH);
    void door(float duration, bool open, int screenW, int screenH);
};

} // namespace Graphics
} // namespace JJM

#endif // TRANSITION_H
