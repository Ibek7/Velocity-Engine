#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "graphics/Texture.h"
#include "graphics/RenderTarget.h"

namespace JJM {
namespace Graphics {

class PostProcessEffect {
public:
    PostProcessEffect();
    virtual ~PostProcessEffect();
    
    virtual void apply(RenderTarget* source, RenderTarget* destination) = 0;
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setIntensity(float intensity) { this->intensity = intensity; }
    float getIntensity() const { return intensity; }

protected:
    bool enabled;
    float intensity;
};

class BloomEffect : public PostProcessEffect {
public:
    BloomEffect();
    ~BloomEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setThreshold(float threshold) { this->threshold = threshold; }
    float getThreshold() const { return threshold; }
    
    void setBlurIterations(int iterations) { this->blurIterations = iterations; }
    int getBlurIterations() const { return blurIterations; }

private:
    float threshold;
    int blurIterations;
    std::unique_ptr<RenderTarget> brightPassTarget;
    std::unique_ptr<RenderTarget> blurTarget;
    
    void extractBrightPixels(RenderTarget* source, RenderTarget* destination);
    void blur(RenderTarget* source, RenderTarget* destination);
    void combine(RenderTarget* source, RenderTarget* bloom, RenderTarget* destination);
};

class BlurEffect : public PostProcessEffect {
public:
    enum BlurType {
        Box,
        Gaussian,
        Radial
    };
    
    BlurEffect();
    ~BlurEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setBlurType(BlurType type) { this->blurType = type; }
    BlurType getBlurType() const { return blurType; }
    
    void setRadius(float radius) { this->radius = radius; }
    float getRadius() const { return radius; }

private:
    BlurType blurType;
    float radius;
    std::unique_ptr<RenderTarget> tempTarget;
    
    void applyBoxBlur(RenderTarget* source, RenderTarget* destination);
    void applyGaussianBlur(RenderTarget* source, RenderTarget* destination);
    void applyRadialBlur(RenderTarget* source, RenderTarget* destination);
};

class ColorGradingEffect : public PostProcessEffect {
public:
    ColorGradingEffect();
    ~ColorGradingEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setExposure(float exposure) { this->exposure = exposure; }
    float getExposure() const { return exposure; }
    
    void setContrast(float contrast) { this->contrast = contrast; }
    float getContrast() const { return contrast; }
    
    void setSaturation(float saturation) { this->saturation = saturation; }
    float getSaturation() const { return saturation; }
    
    void setBrightness(float brightness) { this->brightness = brightness; }
    float getBrightness() const { return brightness; }

private:
    float exposure;
    float contrast;
    float saturation;
    float brightness;
};

class VignetteEffect : public PostProcessEffect {
public:
    VignetteEffect();
    ~VignetteEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setInnerRadius(float radius) { this->innerRadius = radius; }
    float getInnerRadius() const { return innerRadius; }
    
    void setOuterRadius(float radius) { this->outerRadius = radius; }
    float getOuterRadius() const { return outerRadius; }
    
    void setColor(float r, float g, float b) {
        color[0] = r; color[1] = g; color[2] = b;
    }

private:
    float innerRadius;
    float outerRadius;
    float color[3];
};

class ChromaticAberrationEffect : public PostProcessEffect {
public:
    ChromaticAberrationEffect();
    ~ChromaticAberrationEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setOffset(float offset) { this->offset = offset; }
    float getOffset() const { return offset; }

private:
    float offset;
};

class MotionBlurEffect : public PostProcessEffect {
public:
    MotionBlurEffect();
    ~MotionBlurEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setSamples(int samples) { this->samples = samples; }
    int getSamples() const { return samples; }
    
    void setVelocityScale(float scale) { this->velocityScale = scale; }
    float getVelocityScale() const { return velocityScale; }

private:
    int samples;
    float velocityScale;
};

class DepthOfFieldEffect : public PostProcessEffect {
public:
    DepthOfFieldEffect();
    ~DepthOfFieldEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setFocusDistance(float distance) { this->focusDistance = distance; }
    float getFocusDistance() const { return focusDistance; }
    
    void setFocusRange(float range) { this->focusRange = range; }
    float getFocusRange() const { return focusRange; }
    
    void setBlurAmount(float amount) { this->blurAmount = amount; }
    float getBlurAmount() const { return blurAmount; }

private:
    float focusDistance;
    float focusRange;
    float blurAmount;
    std::unique_ptr<RenderTarget> blurTarget;
};

class FilmGrainEffect : public PostProcessEffect {
public:
    FilmGrainEffect();
    ~FilmGrainEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setGrainSize(float size) { this->grainSize = size; }
    float getGrainSize() const { return grainSize; }
    
    void setLuminanceBlending(float blend) { this->luminanceBlending = blend; }
    float getLuminanceBlending() const { return luminanceBlending; }

private:
    float grainSize;
    float luminanceBlending;
    float randomSeed;
};

class PostProcessingStack {
public:
    PostProcessingStack();
    ~PostProcessingStack();
    
    void addEffect(std::shared_ptr<PostProcessEffect> effect);
    void removeEffect(PostProcessEffect* effect);
    void clearEffects();
    
    void apply(RenderTarget* source, RenderTarget* destination);
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    const std::vector<std::shared_ptr<PostProcessEffect>>& getEffects() const {
        return effects;
    }

private:
    std::vector<std::shared_ptr<PostProcessEffect>> effects;
    std::vector<std::unique_ptr<RenderTarget>> pingPongTargets;
    bool enabled;
    
    void ensurePingPongTargets(int width, int height);
};

class ToneMappingEffect : public PostProcessEffect {
public:
    enum ToneMapper {
        Linear,
        Reinhard,
        Filmic,
        ACES
    };
    
    ToneMappingEffect();
    ~ToneMappingEffect();
    
    void apply(RenderTarget* source, RenderTarget* destination) override;
    
    void setToneMapper(ToneMapper mapper) { this->toneMapper = mapper; }
    ToneMapper getToneMapper() const { return toneMapper; }
    
    void setExposure(float exposure) { this->exposure = exposure; }
    float getExposure() const { return exposure; }

private:
    ToneMapper toneMapper;
    float exposure;
    
    float linearToneMap(float hdr);
    float reinhardToneMap(float hdr);
    float filmicToneMap(float hdr);
    float acesToneMap(float hdr);
};

} // namespace Graphics
} // namespace JJM
