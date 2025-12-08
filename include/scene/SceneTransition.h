#pragma once

#include <memory>
#include <functional>
#include <string>

namespace JJM {
namespace Scene {

enum class TransitionType {
    Fade,
    Slide,
    Wipe,
    Dissolve,
    Zoom,
    Custom
};

enum class TransitionDirection {
    Left,
    Right,
    Up,
    Down
};

class SceneTransition {
public:
    SceneTransition(float duration);
    virtual ~SceneTransition() = default;
    
    virtual void update(float deltaTime);
    virtual void render() = 0;
    
    void start();
    void reset();
    
    bool isComplete() const { return progress >= 1.0f; }
    float getProgress() const { return progress; }
    
    void setDuration(float duration) { this->duration = duration; }
    float getDuration() const { return duration; }
    
    void setOnComplete(std::function<void()> callback) { onComplete = callback; }

protected:
    float progress;
    float duration;
    std::function<void()> onComplete;
};

class FadeTransition : public SceneTransition {
public:
    FadeTransition(float duration, bool fadeIn = false);
    
    void render() override;
    
    void setFadeIn(bool fadeIn) { this->fadeIn = fadeIn; }
    void setColor(float r, float g, float b);

private:
    bool fadeIn;
    float color[3];
};

class SlideTransition : public SceneTransition {
public:
    SlideTransition(float duration, TransitionDirection direction);
    
    void render() override;
    
    void setDirection(TransitionDirection direction) { this->direction = direction; }

private:
    TransitionDirection direction;
};

class WipeTransition : public SceneTransition {
public:
    WipeTransition(float duration, TransitionDirection direction);
    
    void render() override;

private:
    TransitionDirection direction;
};

class DissolveTransition : public SceneTransition {
public:
    DissolveTransition(float duration);
    
    void render() override;
    
    void setNoiseScale(float scale) { noiseScale = scale; }

private:
    float noiseScale;
};

class ZoomTransition : public SceneTransition {
public:
    ZoomTransition(float duration, bool zoomIn = false);
    
    void render() override;
    
    void setZoomIn(bool zoomIn) { this->zoomIn = zoomIn; }

private:
    bool zoomIn;
};

class TransitionManager {
public:
    TransitionManager();
    ~TransitionManager();
    
    void startTransition(std::unique_ptr<SceneTransition> transition);
    void update(float deltaTime);
    void render();
    
    bool isTransitioning() const { return currentTransition != nullptr; }
    SceneTransition* getCurrentTransition() { return currentTransition.get(); }
    
    void createFadeTransition(float duration, bool fadeIn = false);
    void createSlideTransition(float duration, TransitionDirection direction);
    void createWipeTransition(float duration, TransitionDirection direction);
    void createDissolveTransition(float duration);
    void createZoomTransition(float duration, bool zoomIn = false);

private:
    std::unique_ptr<SceneTransition> currentTransition;
};

class CrossfadeTransition : public SceneTransition {
public:
    CrossfadeTransition(float duration);
    
    void render() override;
    
    void setFromScene(void* sceneData) { fromScene = sceneData; }
    void setToScene(void* sceneData) { toScene = sceneData; }

private:
    void* fromScene;
    void* toScene;
};

} // namespace Scene
} // namespace JJM
