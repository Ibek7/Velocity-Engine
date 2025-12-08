#include "scene/SceneTransition.h"
#include <algorithm>

namespace JJM {
namespace Scene {

SceneTransition::SceneTransition(float duration)
    : progress(0.0f), duration(duration) {}

void SceneTransition::update(float deltaTime) {
    if (progress < 1.0f) {
        progress += deltaTime / duration;
        progress = std::min(1.0f, progress);
        
        if (progress >= 1.0f && onComplete) {
            onComplete();
        }
    }
}

void SceneTransition::start() {
    progress = 0.0f;
}

void SceneTransition::reset() {
    progress = 0.0f;
}

FadeTransition::FadeTransition(float duration, bool fadeIn)
    : SceneTransition(duration), fadeIn(fadeIn) {
    color[0] = 0.0f;
    color[1] = 0.0f;
    color[2] = 0.0f;
}

void FadeTransition::render() {
    float alpha = fadeIn ? (1.0f - progress) : progress;
    (void)alpha;
}

void FadeTransition::setColor(float r, float g, float b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

SlideTransition::SlideTransition(float duration, TransitionDirection direction)
    : SceneTransition(duration), direction(direction) {}

void SlideTransition::render() {
    float offset = 0.0f;
    
    switch (direction) {
        case TransitionDirection::Left:
            offset = -progress * 1920.0f;
            break;
        case TransitionDirection::Right:
            offset = progress * 1920.0f;
            break;
        case TransitionDirection::Up:
            offset = -progress * 1080.0f;
            break;
        case TransitionDirection::Down:
            offset = progress * 1080.0f;
            break;
    }
    
    (void)offset;
}

WipeTransition::WipeTransition(float duration, TransitionDirection direction)
    : SceneTransition(duration), direction(direction) {}

void WipeTransition::render() {
    float wipePosition = progress;
    (void)wipePosition;
    (void)direction;
}

DissolveTransition::DissolveTransition(float duration)
    : SceneTransition(duration), noiseScale(1.0f) {}

void DissolveTransition::render() {
    float threshold = progress;
    (void)threshold;
    (void)noiseScale;
}

ZoomTransition::ZoomTransition(float duration, bool zoomIn)
    : SceneTransition(duration), zoomIn(zoomIn) {}

void ZoomTransition::render() {
    float scale = zoomIn ? (1.0f - progress) : (1.0f + progress * 2.0f);
    (void)scale;
}

TransitionManager::TransitionManager() {}

TransitionManager::~TransitionManager() {}

void TransitionManager::startTransition(std::unique_ptr<SceneTransition> transition) {
    if (transition) {
        transition->start();
        currentTransition = std::move(transition);
    }
}

void TransitionManager::update(float deltaTime) {
    if (currentTransition) {
        currentTransition->update(deltaTime);
        
        if (currentTransition->isComplete()) {
            currentTransition.reset();
        }
    }
}

void TransitionManager::render() {
    if (currentTransition) {
        currentTransition->render();
    }
}

void TransitionManager::createFadeTransition(float duration, bool fadeIn) {
    auto transition = std::make_unique<FadeTransition>(duration, fadeIn);
    startTransition(std::move(transition));
}

void TransitionManager::createSlideTransition(float duration, TransitionDirection direction) {
    auto transition = std::make_unique<SlideTransition>(duration, direction);
    startTransition(std::move(transition));
}

void TransitionManager::createWipeTransition(float duration, TransitionDirection direction) {
    auto transition = std::make_unique<WipeTransition>(duration, direction);
    startTransition(std::move(transition));
}

void TransitionManager::createDissolveTransition(float duration) {
    auto transition = std::make_unique<DissolveTransition>(duration);
    startTransition(std::move(transition));
}

void TransitionManager::createZoomTransition(float duration, bool zoomIn) {
    auto transition = std::make_unique<ZoomTransition>(duration, zoomIn);
    startTransition(std::move(transition));
}

CrossfadeTransition::CrossfadeTransition(float duration)
    : SceneTransition(duration), fromScene(nullptr), toScene(nullptr) {}

void CrossfadeTransition::render() {
    float alpha = progress;
    (void)alpha;
    (void)fromScene;
    (void)toScene;
}

} // namespace Scene
} // namespace JJM
