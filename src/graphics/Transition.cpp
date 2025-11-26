#include "graphics/Transition.h"
#include "math/Vector2D.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// Transition base class
Transition::Transition(TransitionType type, float duration)
    : type(type), duration(duration), elapsed(0), finished(false) {
}

void Transition::update(float deltaTime) {
    if (finished) return;
    
    elapsed += deltaTime;
    
    if (elapsed >= duration) {
        elapsed = duration;
        finished = true;
        
        if (onComplete) {
            onComplete();
        }
    }
}

void Transition::reset() {
    elapsed = 0;
    finished = false;
}

float Transition::getProgress() const {
    return std::min(elapsed / duration, 1.0f);
}

// FadeTransition
FadeTransition::FadeTransition(float duration, const Color& color, bool fadeIn)
    : Transition(TransitionType::FADE, duration), color(color), fadeIn(fadeIn) {
}

void FadeTransition::render(Renderer* renderer) {
    if (!renderer) return;
    
    float progress = getProgress();
    float alpha = fadeIn ? (1.0f - progress) : progress;
    
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
        color.r, color.g, color.b,
        static_cast<Uint8>(alpha * 255));
    SDL_RenderFillRect(renderer->getSDLRenderer(), nullptr);
}

// SlideTransition
SlideTransition::SlideTransition(TransitionType type, float duration, int screenW, int screenH)
    : Transition(type, duration), screenWidth(screenW), screenHeight(screenH) {
    
    switch (type) {
        case TransitionType::SLIDE_LEFT:
            direction = Math::Vector2D(-1, 0);
            break;
        case TransitionType::SLIDE_RIGHT:
            direction = Math::Vector2D(1, 0);
            break;
        case TransitionType::SLIDE_UP:
            direction = Math::Vector2D(0, -1);
            break;
        case TransitionType::SLIDE_DOWN:
            direction = Math::Vector2D(0, 1);
            break;
        default:
            direction = Math::Vector2D(0, 0);
            break;
    }
}

void SlideTransition::render(Renderer* renderer) {
    if (!renderer) return;
    
    float progress = getProgress();
    
    int offsetX = static_cast<int>(direction.x * screenWidth * progress);
    int offsetY = static_cast<int>(direction.y * screenHeight * progress);
    
    SDL_Rect rect;
    
    if (direction.x < 0) {
        rect = {screenWidth + offsetX, 0, -offsetX, screenHeight};
    } else if (direction.x > 0) {
        rect = {0, 0, offsetX, screenHeight};
    } else if (direction.y < 0) {
        rect = {0, screenHeight + offsetY, screenWidth, -offsetY};
    } else if (direction.y > 0) {
        rect = {0, 0, screenWidth, offsetY};
    }
    
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0, 255);
    SDL_RenderFillRect(renderer->getSDLRenderer(), &rect);
}

// WipeTransition
WipeTransition::WipeTransition(bool leftToRight, float duration, int screenW, int screenH)
    : Transition(leftToRight ? TransitionType::WIPE_RIGHT : TransitionType::WIPE_LEFT, duration),
      leftToRight(leftToRight), screenWidth(screenW), screenHeight(screenH) {
}

void WipeTransition::render(Renderer* renderer) {
    if (!renderer) return;
    
    float progress = getProgress();
    int width = static_cast<int>(screenWidth * progress);
    
    SDL_Rect rect;
    if (leftToRight) {
        rect = {0, 0, width, screenHeight};
    } else {
        rect = {screenWidth - width, 0, width, screenHeight};
    }
    
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0, 255);
    SDL_RenderFillRect(renderer->getSDLRenderer(), &rect);
}

// CircleTransition
CircleTransition::CircleTransition(bool opening, float duration, int screenW, int screenH)
    : Transition(opening ? TransitionType::CIRCLE_OPEN : TransitionType::CIRCLE_CLOSE, duration),
      opening(opening),
      center(screenW / 2.0f, screenH / 2.0f),
      maxRadius(std::sqrt(screenW * screenW + screenH * screenH) / 2.0f) {
}

CircleTransition::CircleTransition(bool opening, float duration,
                                 const Math::Vector2D& center, float maxRadius)
    : Transition(opening ? TransitionType::CIRCLE_OPEN : TransitionType::CIRCLE_CLOSE, duration),
      opening(opening), center(center), maxRadius(maxRadius) {
}

void CircleTransition::render(Renderer* renderer) {
    if (!renderer) return;
    
    float progress = getProgress();
    float radius = opening ? (maxRadius * (1.0f - progress)) : (maxRadius * progress);
    
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0, 255);
    
    if (opening) {
        renderCircle(renderer, center, radius);
    } else {
        // For closing, we need to render everything except the circle
        // This is a simplified version
        renderCircle(renderer, center, radius);
    }
}

void CircleTransition::renderCircle(Renderer* renderer, const Math::Vector2D& center, float radius) {
    int segments = 64;
    float angleStep = (2.0f * M_PI) / segments;
    
    for (int i = 0; i < segments; i++) {
        float angle1 = i * angleStep;
        float angle2 = (i + 1) * angleStep;
        
        int x1 = static_cast<int>(center.x + radius * std::cos(angle1));
        int y1 = static_cast<int>(center.y + radius * std::sin(angle1));
        int x2 = static_cast<int>(center.x + radius * std::cos(angle2));
        int y2 = static_cast<int>(center.y + radius * std::sin(angle2));
        
        SDL_RenderDrawLine(renderer->getSDLRenderer(),
            center.x, center.y, x1, y1);
    }
}

// PixelateTransition
PixelateTransition::PixelateTransition(float duration, int screenW, int screenH, int maxPixelSize)
    : Transition(TransitionType::PIXELATE, duration),
      screenWidth(screenW), screenHeight(screenH), maxPixelSize(maxPixelSize) {
}

void PixelateTransition::render(Renderer* renderer) {
    if (!renderer) return;
    
    float progress = getProgress();
    int pixelSize = static_cast<int>(1 + (maxPixelSize - 1) * progress);
    
    // Simplified pixelate effect
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0,
        static_cast<Uint8>(progress * 128));
    
    for (int y = 0; y < screenHeight; y += pixelSize) {
        for (int x = 0; x < screenWidth; x += pixelSize) {
            SDL_Rect rect = {x, y, pixelSize, pixelSize};
            SDL_RenderFillRect(renderer->getSDLRenderer(), &rect);
        }
    }
}

// TransitionManager
TransitionManager* TransitionManager::instance = nullptr;

TransitionManager::TransitionManager() : currentTransition(nullptr) {
}

TransitionManager* TransitionManager::getInstance() {
    if (!instance) {
        instance = new TransitionManager();
    }
    return instance;
}

TransitionManager::~TransitionManager() {
    clearTransition();
}

void TransitionManager::update(float deltaTime) {
    if (currentTransition) {
        currentTransition->update(deltaTime);
        
        if (currentTransition->isFinished()) {
            clearTransition();
        }
    }
}

void TransitionManager::render(Renderer* renderer) {
    if (currentTransition && renderer) {
        currentTransition->render(renderer);
    }
}

void TransitionManager::startTransition(Transition* transition) {
    clearTransition();
    currentTransition = transition;
}

void TransitionManager::clearTransition() {
    if (currentTransition) {
        delete currentTransition;
        currentTransition = nullptr;
    }
}

void TransitionManager::fadeOut(float duration, const Color& color) {
    startTransition(new FadeTransition(duration, color, false));
}

void TransitionManager::fadeIn(float duration, const Color& color) {
    startTransition(new FadeTransition(duration, color, true));
}

void TransitionManager::slideLeft(float duration, int screenW, int screenH) {
    startTransition(new SlideTransition(TransitionType::SLIDE_LEFT, duration, screenW, screenH));
}

void TransitionManager::slideRight(float duration, int screenW, int screenH) {
    startTransition(new SlideTransition(TransitionType::SLIDE_RIGHT, duration, screenW, screenH));
}

void TransitionManager::circleClose(float duration, int screenW, int screenH) {
    startTransition(new CircleTransition(false, duration, screenW, screenH));
}

void TransitionManager::circleOpen(float duration, int screenW, int screenH) {
    startTransition(new CircleTransition(true, duration, screenW, screenH));
}

} // namespace Graphics
} // namespace JJM
