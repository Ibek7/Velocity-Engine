#include "graphics/PostProcessing.h"
#include <cmath>
#include <algorithm>

namespace JJM {
namespace Graphics {

// PostEffect base class
PostEffect::PostEffect(PostEffectType type, float intensity)
    : type(type), enabled(true), intensity(intensity) {
}

// GrayscaleEffect
GrayscaleEffect::GrayscaleEffect(float intensity)
    : PostEffect(PostEffectType::GRAYSCALE, intensity) {
}

void GrayscaleEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    // Simplified grayscale overlay
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureColorMod(source, 128, 128, 128);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// SepiaEffect
SepiaEffect::SepiaEffect(float intensity)
    : PostEffect(PostEffectType::SEPIA, intensity) {
}

void SepiaEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_MOD);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(),
        static_cast<Uint8>(255 * intensity),
        static_cast<Uint8>(240 * intensity),
        static_cast<Uint8>(200 * intensity),
        255);
    SDL_RenderFillRect(renderer->getSDLRenderer(), nullptr);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// InvertEffect
InvertEffect::InvertEffect(float intensity)
    : PostEffect(PostEffectType::INVERT, intensity) {
}

void InvertEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    
    // Simplified invert effect
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 255, 255, 255, 255);
    SDL_RenderFillRect(renderer->getSDLRenderer(), nullptr);
    
    SDL_SetTextureBlendMode(source, SDL_BLENDMODE_MOD);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// BrightnessEffect
BrightnessEffect::BrightnessEffect(float brightness)
    : PostEffect(PostEffectType::BRIGHTNESS, 1.0f), brightness(brightness) {
}

void BrightnessEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    
    Uint8 colorMod = static_cast<Uint8>(std::min(255.0f, 255.0f * brightness));
    SDL_SetTextureColorMod(source, colorMod, colorMod, colorMod);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    SDL_SetTextureColorMod(source, 255, 255, 255);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// ContrastEffect
ContrastEffect::ContrastEffect(float contrast)
    : PostEffect(PostEffectType::CONTRAST, 1.0f), contrast(contrast) {
}

void ContrastEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// VignetteEffect
VignetteEffect::VignetteEffect(float intensity, float radius, float softness)
    : PostEffect(PostEffectType::VIGNETTE, intensity),
      radius(radius), softness(softness) {
}

void VignetteEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    // Simple vignette overlay
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    
    int w, h;
    SDL_QueryTexture(destination, nullptr, nullptr, &w, &h);
    
    int margin = static_cast<int>((1.0f - radius) * std::min(w, h) / 2);
    Uint8 alpha = static_cast<Uint8>(intensity * 128);
    
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0, alpha);
    
    SDL_Rect rects[4] = {
        {0, 0, w, margin},
        {0, h - margin, w, margin},
        {0, margin, margin, h - 2 * margin},
        {w - margin, margin, margin, h - 2 * margin}
    };
    
    for (const auto& rect : rects) {
        SDL_RenderFillRect(renderer->getSDLRenderer(), &rect);
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// ScanlinesEffect
ScanlinesEffect::ScanlinesEffect(int spacing, float lineIntensity)
    : PostEffect(PostEffectType::SCANLINES, 1.0f),
      lineSpacing(spacing), lineIntensity(lineIntensity) {
}

void ScanlinesEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    SDL_RenderCopy(renderer->getSDLRenderer(), source, nullptr, nullptr);
    
    int w, h;
    SDL_QueryTexture(destination, nullptr, nullptr, &w, &h);
    
    SDL_SetRenderDrawBlendMode(renderer->getSDLRenderer(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer->getSDLRenderer(), 0, 0, 0,
        static_cast<Uint8>(lineIntensity * 255));
    
    for (int y = 0; y < h; y += lineSpacing) {
        SDL_RenderDrawLine(renderer->getSDLRenderer(), 0, y, w, y);
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// PixelateEffect
PixelateEffect::PixelateEffect(int pixelSize)
    : PostEffect(PostEffectType::PIXELATE, 1.0f), pixelSize(pixelSize) {
}

void PixelateEffect::apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) {
    if (!enabled || !renderer || !source || !destination) return;
    
    int w, h;
    SDL_QueryTexture(source, nullptr, nullptr, &w, &h);
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), destination);
    
    // Render at lower resolution then scale up
    for (int y = 0; y < h; y += pixelSize) {
        for (int x = 0; x < w; x += pixelSize) {
            SDL_Rect src = {x, y, 1, 1};
            SDL_Rect dst = {x, y, pixelSize, pixelSize};
            SDL_RenderCopy(renderer->getSDLRenderer(), source, &src, &dst);
        }
    }
    
    SDL_SetRenderTarget(renderer->getSDLRenderer(), nullptr);
}

// PostProcessingPipeline
PostProcessingPipeline::PostProcessingPipeline(int width, int height)
    : width(width), height(height), bufferA(nullptr), bufferB(nullptr),
      enabled(true) {
}

PostProcessingPipeline::~PostProcessingPipeline() {
    clearEffects();
    destroyBuffers();
}

void PostProcessingPipeline::addEffect(PostEffect* effect) {
    if (effect) {
        effects.push_back(effect);
    }
}

void PostProcessingPipeline::removeEffect(PostEffect* effect) {
    auto it = std::find(effects.begin(), effects.end(), effect);
    if (it != effects.end()) {
        effects.erase(it);
    }
}

void PostProcessingPipeline::clearEffects() {
    for (auto* effect : effects) {
        delete effect;
    }
    effects.clear();
}

void PostProcessingPipeline::process(SDL_Texture* source, Renderer* renderer) {
    if (!enabled || effects.empty() || !renderer || !source) return;
    
    if (!bufferA || !bufferB) {
        createBuffers(renderer);
    }
    
    SDL_Texture* currentSource = source;
    SDL_Texture* currentDest = bufferA;
    
    for (size_t i = 0; i < effects.size(); i++) {
        if (effects[i]->isEnabled()) {
            effects[i]->apply(currentSource, currentDest, renderer);
            
            std::swap(currentSource, currentDest);
            
            if (i == effects.size() - 1) {
                currentDest = nullptr;
            }
        }
    }
}

void PostProcessingPipeline::apply(Renderer* renderer) {
    if (!enabled || !renderer || !bufferA) return;
    
    SDL_RenderCopy(renderer->getSDLRenderer(), bufferA, nullptr, nullptr);
}

void PostProcessingPipeline::resize(int newWidth, int newHeight, Renderer* renderer) {
    width = newWidth;
    height = newHeight;
    
    destroyBuffers();
    createBuffers(renderer);
}

void PostProcessingPipeline::createBuffers(Renderer* renderer) {
    if (!renderer) return;
    
    bufferA = SDL_CreateTexture(renderer->getSDLRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height);
    
    bufferB = SDL_CreateTexture(renderer->getSDLRenderer(),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        width, height);
}

void PostProcessingPipeline::destroyBuffers() {
    if (bufferA) {
        SDL_DestroyTexture(bufferA);
        bufferA = nullptr;
    }
    if (bufferB) {
        SDL_DestroyTexture(bufferB);
        bufferB = nullptr;
    }
}

} // namespace Graphics
} // namespace JJM
