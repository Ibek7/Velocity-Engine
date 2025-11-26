#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <SDL2/SDL.h>
#include <vector>

namespace JJM {
namespace Graphics {

enum class PostEffectType {
    GRAYSCALE,
    SEPIA,
    INVERT,
    BRIGHTNESS,
    CONTRAST,
    BLUR,
    VIGNETTE,
    SCANLINES,
    CHROMATIC_ABERRATION,
    PIXELATE
};

class PostEffect {
protected:
    PostEffectType type;
    bool enabled;
    float intensity;
    
public:
    PostEffect(PostEffectType type, float intensity = 1.0f);
    virtual ~PostEffect() = default;
    
    virtual void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) = 0;
    
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }
    
    void setIntensity(float i) { intensity = i; }
    float getIntensity() const { return intensity; }
    
    PostEffectType getType() const { return type; }
};

class GrayscaleEffect : public PostEffect {
public:
    GrayscaleEffect(float intensity = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
};

class SepiaEffect : public PostEffect {
public:
    SepiaEffect(float intensity = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
};

class InvertEffect : public PostEffect {
public:
    InvertEffect(float intensity = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
};

class BrightnessEffect : public PostEffect {
private:
    float brightness;
    
public:
    BrightnessEffect(float brightness = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setBrightness(float b) { brightness = b; }
    float getBrightness() const { return brightness; }
};

class ContrastEffect : public PostEffect {
private:
    float contrast;
    
public:
    ContrastEffect(float contrast = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setContrast(float c) { contrast = c; }
    float getContrast() const { return contrast; }
};

class VignetteEffect : public PostEffect {
private:
    float radius;
    float softness;
    
public:
    VignetteEffect(float intensity = 1.0f, float radius = 0.8f, float softness = 0.5f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setRadius(float r) { radius = r; }
    void setSoftness(float s) { softness = s; }
};

class ScanlinesEffect : public PostEffect {
private:
    int lineSpacing;
    float lineIntensity;
    
public:
    ScanlinesEffect(int spacing = 4, float lineIntensity = 0.5f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
};

class PixelateEffect : public PostEffect {
private:
    int pixelSize;
    
public:
    PixelateEffect(int pixelSize = 4);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setPixelSize(int size) { pixelSize = size; }
    int getPixelSize() const { return pixelSize; }
};

class PostProcessingPipeline {
private:
    std::vector<PostEffect*> effects;
    SDL_Texture* bufferA;
    SDL_Texture* bufferB;
    int width;
    int height;
    bool enabled;
    
public:
    PostProcessingPipeline(int width, int height);
    ~PostProcessingPipeline();
    
    void addEffect(PostEffect* effect);
    void removeEffect(PostEffect* effect);
    void clearEffects();
    
    void process(SDL_Texture* source, Renderer* renderer);
    void apply(Renderer* renderer);
    
    void setEnabled(bool e) { enabled = e; }
    bool isEnabled() const { return enabled; }
    
    int getEffectCount() const { return static_cast<int>(effects.size()); }
    
    void resize(int newWidth, int newHeight, Renderer* renderer);
    
private:
    void createBuffers(Renderer* renderer);
    void destroyBuffers();
};

} // namespace Graphics
} // namespace JJM

#endif // POST_PROCESSING_H
