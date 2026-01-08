#ifndef POST_PROCESSING_H
#define POST_PROCESSING_H

#include "graphics/Renderer.h"
#include "graphics/Color.h"
#include <SDL.h>
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
    PIXELATE,
    BLOOM,
    COLOR_GRADING,
    TONE_MAPPING,
    DEPTH_OF_FIELD,
    MOTION_BLUR
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

/**
 * @brief Multi-pass bloom effect with threshold and blur
 */
class BloomEffect : public PostEffect {
private:
    float threshold;       // Brightness threshold for bloom
    float blurRadius;      // Blur spread
    int passes;            // Number of blur passes
    
public:
    BloomEffect(float threshold = 0.8f, float blurRadius = 5.0f, int passes = 2);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setThreshold(float t) { threshold = t; }
    float getThreshold() const { return threshold; }
    
    void setBlurRadius(float r) { blurRadius = r; }
    float getBlurRadius() const { return blurRadius; }
    
    void setPasses(int p) { passes = p; }
    int getPasses() const { return passes; }
};

/**
 * @brief Color grading with adjustable curves and color temperature
 */
class ColorGradingEffect : public PostEffect {
public:
    struct ColorCurve {
        float shadows;     // Adjustment for dark areas (-1 to 1)
        float midtones;    // Adjustment for mid-range (0 to 2)
        float highlights;  // Adjustment for bright areas (0 to 2)
    };
    
private:
    ColorCurve redCurve;
    ColorCurve greenCurve;
    ColorCurve blueCurve;
    float saturation;      // Overall saturation multiplier
    float temperature;     // Color temperature (-1=cool, 0=neutral, 1=warm)
    float tint;           // Green-magenta tint
    
public:
    ColorGradingEffect(float intensity = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setRedCurve(const ColorCurve& curve) { redCurve = curve; }
    void setGreenCurve(const ColorCurve& curve) { greenCurve = curve; }
    void setBlueCurve(const ColorCurve& curve) { blueCurve = curve; }
    
    void setSaturation(float s) { saturation = s; }
    float getSaturation() const { return saturation; }
    
    void setTemperature(float t) { temperature = t; }
    float getTemperature() const { return temperature; }
    
    void setTint(float t) { tint = t; }
    float getTint() const { return tint; }
};

/**
 * @brief Tone mapping for HDR to LDR conversion
 */
class ToneMappingEffect : public PostEffect {
public:
    enum class ToneMapMode {
        REINHARD,          // Simple Reinhard
        REINHARD_EXTENDED, // Reinhard with white point
        UNCHARTED2,        // John Hable's Uncharted 2
        ACES,              // ACES filmic
        EXPOSURE           // Simple exposure adjustment
    };
    
private:
    ToneMapMode mode;
    float exposure;        // Exposure adjustment
    float whitePoint;      // White point for extended Reinhard
    float gamma;           // Gamma correction
    
public:
    ToneMappingEffect(ToneMapMode mode = ToneMapMode::ACES, float exposure = 1.0f);
    void apply(SDL_Texture* source, SDL_Texture* destination, Renderer* renderer) override;
    
    void setMode(ToneMapMode m) { mode = m; }
    ToneMapMode getMode() const { return mode; }
    
    void setExposure(float e) { exposure = e; }
    float getExposure() const { return exposure; }
    
    void setWhitePoint(float w) { whitePoint = w; }
    float getWhitePoint() const { return whitePoint; }
    
    void setGamma(float g) { gamma = g; }
    float getGamma() const { return gamma; }
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
    
    // Effect chain configuration
    void insertEffect(size_t index, PostEffect* effect);
    void moveEffect(size_t fromIndex, size_t toIndex);
    PostEffect* getEffect(size_t index) const;
    
    // Render target management for multi-pass effects
    void addRenderTarget(const std::string& name, int width, int height, Renderer* renderer);
    SDL_Texture* getRenderTarget(const std::string& name) const;
    void clearRenderTargets();
    
    // Effect presets
    void loadPreset(const std::string& presetName);
    void savePreset(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
private:
    void createBuffers(Renderer* renderer);
    void destroyBuffers();
    
    // Multi-pass rendering
    std::unordered_map<std::string, SDL_Texture*> m_renderTargets;
};

} // namespace Graphics
} // namespace JJM

#endif // POST_PROCESSING_H
