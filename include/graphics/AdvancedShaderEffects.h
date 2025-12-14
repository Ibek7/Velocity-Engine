#pragma once

#include "graphics/ShaderSystem.h"
#include "math/Vector2D.h"
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace JJM {
namespace Graphics {

enum class ShaderEffectType {
    Distortion,
    Ripple,
    Wave,
    Pixelate,
    EdgeDetection,
    Emboss,
    Sharpen,
    Outline,
    Glow,
    Dissolve
};

class ShaderEffect {
public:
    ShaderEffect();
    virtual ~ShaderEffect();
    
    virtual void apply(Shader* shader) = 0;
    virtual ShaderEffectType getType() const = 0;
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setIntensity(float intensity) { this->intensity = intensity; }
    float getIntensity() const { return intensity; }

protected:
    bool enabled;
    float intensity;
};

class DistortionEffect : public ShaderEffect {
public:
    DistortionEffect();
    ~DistortionEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Distortion; }
    
    void setDistortionAmount(float amount) { distortionAmount = amount; }
    float getDistortionAmount() const { return distortionAmount; }
    
    void setCenter(const Math::Vector2D& center) { this->center = center; }
    const Math::Vector2D& getCenter() const { return center; }

private:
    float distortionAmount;
    Math::Vector2D center;
};

class RippleEffect : public ShaderEffect {
public:
    RippleEffect();
    ~RippleEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Ripple; }
    
    void setWaveCount(int count) { waveCount = count; }
    int getWaveCount() const { return waveCount; }
    
    void setAmplitude(float amplitude) { this->amplitude = amplitude; }
    float getAmplitude() const { return amplitude; }
    
    void setFrequency(float frequency) { this->frequency = frequency; }
    float getFrequency() const { return frequency; }
    
    void setTime(float time) { this->time = time; }
    float getTime() const { return time; }
    
    void setCenter(const Math::Vector2D& center) { this->center = center; }
    const Math::Vector2D& getCenter() const { return center; }

private:
    int waveCount;
    float amplitude;
    float frequency;
    float time;
    Math::Vector2D center;
};

class WaveEffect : public ShaderEffect {
public:
    WaveEffect();
    ~WaveEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Wave; }
    
    void setAmplitude(float amplitude) { this->amplitude = amplitude; }
    float getAmplitude() const { return amplitude; }
    
    void setFrequency(float frequency) { this->frequency = frequency; }
    float getFrequency() const { return frequency; }
    
    void setSpeed(float speed) { this->speed = speed; }
    float getSpeed() const { return speed; }
    
    void setTime(float time) { this->time = time; }
    float getTime() const { return time; }
    
    void setDirection(const Math::Vector2D& direction) { this->direction = direction; }
    const Math::Vector2D& getDirection() const { return direction; }

private:
    float amplitude;
    float frequency;
    float speed;
    float time;
    Math::Vector2D direction;
};

class PixelateEffect : public ShaderEffect {
public:
    PixelateEffect();
    ~PixelateEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Pixelate; }
    
    void setPixelSize(int size) { pixelSize = size; }
    int getPixelSize() const { return pixelSize; }

private:
    int pixelSize;
};

class EdgeDetectionEffect : public ShaderEffect {
public:
    EdgeDetectionEffect();
    ~EdgeDetectionEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::EdgeDetection; }
    
    void setThreshold(float threshold) { this->threshold = threshold; }
    float getThreshold() const { return threshold; }
    
    void setEdgeColor(float r, float g, float b) {
        edgeColor[0] = r; edgeColor[1] = g; edgeColor[2] = b;
    }

private:
    float threshold;
    float edgeColor[3];
};

class EmbossEffect : public ShaderEffect {
public:
    EmbossEffect();
    ~EmbossEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Emboss; }
    
    void setStrength(float strength) { this->strength = strength; }
    float getStrength() const { return strength; }

private:
    float strength;
};

class SharpenEffect : public ShaderEffect {
public:
    SharpenEffect();
    ~SharpenEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Sharpen; }
    
    void setAmount(float amount) { this->amount = amount; }
    float getAmount() const { return amount; }

private:
    float amount;
};

class OutlineEffect : public ShaderEffect {
public:
    OutlineEffect();
    ~OutlineEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Outline; }
    
    void setOutlineWidth(float width) { outlineWidth = width; }
    float getOutlineWidth() const { return outlineWidth; }
    
    void setOutlineColor(float r, float g, float b, float a) {
        outlineColor[0] = r; outlineColor[1] = g;
        outlineColor[2] = b; outlineColor[3] = a;
    }

private:
    float outlineWidth;
    float outlineColor[4];
};

class GlowEffect : public ShaderEffect {
public:
    GlowEffect();
    ~GlowEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Glow; }
    
    void setGlowRadius(float radius) { glowRadius = radius; }
    float getGlowRadius() const { return glowRadius; }
    
    void setGlowColor(float r, float g, float b) {
        glowColor[0] = r; glowColor[1] = g; glowColor[2] = b;
    }

private:
    float glowRadius;
    float glowColor[3];
};

class DissolveEffect : public ShaderEffect {
public:
    DissolveEffect();
    ~DissolveEffect();
    
    void apply(Shader* shader) override;
    ShaderEffectType getType() const override { return ShaderEffectType::Dissolve; }
    
    void setDissolveAmount(float amount) { dissolveAmount = amount; }
    float getDissolveAmount() const { return dissolveAmount; }
    
    void setEdgeWidth(float width) { edgeWidth = width; }
    float getEdgeWidth() const { return edgeWidth; }
    
    void setEdgeColor(float r, float g, float b) {
        edgeColor[0] = r; edgeColor[1] = g; edgeColor[2] = b;
    }

private:
    float dissolveAmount;
    float edgeWidth;
    float edgeColor[3];
};

class ShaderEffectChain {
public:
    ShaderEffectChain();
    ~ShaderEffectChain();
    
    void addEffect(std::shared_ptr<ShaderEffect> effect);
    void removeEffect(ShaderEffectType type);
    void clearEffects();
    
    std::shared_ptr<ShaderEffect> getEffect(ShaderEffectType type);
    
    void apply(Shader* shader);
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    std::vector<std::shared_ptr<ShaderEffect>> effects;
    bool enabled;
};

class ShaderEffectLibrary {
public:
    static ShaderEffectLibrary& getInstance();
    
    void registerEffect(const std::string& name, std::shared_ptr<ShaderEffect> effect);
    void unregisterEffect(const std::string& name);
    
    std::shared_ptr<ShaderEffect> getEffect(const std::string& name);
    
    void clearEffects();

private:
    ShaderEffectLibrary() {}
    ~ShaderEffectLibrary() {}
    ShaderEffectLibrary(const ShaderEffectLibrary&) = delete;
    ShaderEffectLibrary& operator=(const ShaderEffectLibrary&) = delete;
    
    std::unordered_map<std::string, std::shared_ptr<ShaderEffect>> effects;
};

} // namespace Graphics
} // namespace JJM
