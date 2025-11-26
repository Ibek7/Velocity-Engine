#ifndef TRANSITION_H
#define TRANSITION_H

#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <functional>
#include <SDL2/SDL.h>

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

class TransitionManager {
private:
    Transition* currentTransition;
    
    static TransitionManager* instance;
    TransitionManager();
    
public:
    static TransitionManager* getInstance();
    ~TransitionManager();
    
    void update(float deltaTime);
    void render(Renderer* renderer);
    
    void startTransition(Transition* transition);
    void clearTransition();
    
    bool isTransitioning() const { return currentTransition != nullptr; }
    Transition* getCurrentTransition() { return currentTransition; }
    
    // Convenience methods
    void fadeOut(float duration, const Color& color = Color::Black());
    void fadeIn(float duration, const Color& color = Color::Black());
    void slideLeft(float duration, int screenW, int screenH);
    void slideRight(float duration, int screenW, int screenH);
    void circleClose(float duration, int screenW, int screenH);
    void circleOpen(float duration, int screenW, int screenH);
};

} // namespace Graphics
} // namespace JJM

#endif // TRANSITION_H
