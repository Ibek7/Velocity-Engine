#include "graphics/AdvancedShaderEffects.h"
#include <cmath>

namespace JJM {
namespace Graphics {

// ShaderEffect implementation
ShaderEffect::ShaderEffect() : enabled(true), intensity(1.0f) {}

ShaderEffect::~ShaderEffect() {}

// DistortionEffect implementation
DistortionEffect::DistortionEffect() : distortionAmount(0.1f), center(0.5f, 0.5f) {}

DistortionEffect::~DistortionEffect() {}

void DistortionEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_distortionAmount", distortionAmount * intensity);
    shader->setUniform("u_distortionCenter", center.x, center.y);
}

// RippleEffect implementation
RippleEffect::RippleEffect()
    : waveCount(3), amplitude(0.01f), frequency(10.0f),
      time(0.0f), center(0.5f, 0.5f) {}

RippleEffect::~RippleEffect() {}

void RippleEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_waveCount", waveCount);
    shader->setUniform("u_amplitude", amplitude * intensity);
    shader->setUniform("u_frequency", frequency);
    shader->setUniform("u_time", time);
    shader->setUniform("u_rippleCenter", center.x, center.y);
}

// WaveEffect implementation
WaveEffect::WaveEffect()
    : amplitude(0.01f), frequency(5.0f), speed(1.0f),
      time(0.0f), direction(1.0f, 0.0f) {}

WaveEffect::~WaveEffect() {}

void WaveEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_waveAmplitude", amplitude * intensity);
    shader->setUniform("u_waveFrequency", frequency);
    shader->setUniform("u_waveSpeed", speed);
    shader->setUniform("u_waveTime", time);
    shader->setUniform("u_waveDirection", direction.x, direction.y);
}

// PixelateEffect implementation
PixelateEffect::PixelateEffect() : pixelSize(8) {}

PixelateEffect::~PixelateEffect() {}

void PixelateEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_pixelSize", pixelSize);
}

// EdgeDetectionEffect implementation
EdgeDetectionEffect::EdgeDetectionEffect() : threshold(0.1f) {
    edgeColor[0] = 1.0f;
    edgeColor[1] = 1.0f;
    edgeColor[2] = 1.0f;
}

EdgeDetectionEffect::~EdgeDetectionEffect() {}

void EdgeDetectionEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_edgeThreshold", threshold * intensity);
    shader->setUniform("u_edgeColor", edgeColor[0], edgeColor[1], edgeColor[2]);
}

// EmbossEffect implementation
EmbossEffect::EmbossEffect() : strength(1.0f) {}

EmbossEffect::~EmbossEffect() {}

void EmbossEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_embossStrength", strength * intensity);
}

// SharpenEffect implementation
SharpenEffect::SharpenEffect() : amount(1.0f) {}

SharpenEffect::~SharpenEffect() {}

void SharpenEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_sharpenAmount", amount * intensity);
}

// OutlineEffect implementation
OutlineEffect::OutlineEffect() : outlineWidth(2.0f) {
    outlineColor[0] = 1.0f;
    outlineColor[1] = 1.0f;
    outlineColor[2] = 0.0f;
    outlineColor[3] = 1.0f;
}

OutlineEffect::~OutlineEffect() {}

void OutlineEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_outlineWidth", outlineWidth);
    shader->setUniform("u_outlineColor", outlineColor[0], outlineColor[1], 
                      outlineColor[2], outlineColor[3]);
}

// GlowEffect implementation
GlowEffect::GlowEffect() : glowRadius(5.0f) {
    glowColor[0] = 1.0f;
    glowColor[1] = 1.0f;
    glowColor[2] = 1.0f;
}

GlowEffect::~GlowEffect() {}

void GlowEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_glowRadius", glowRadius * intensity);
    shader->setUniform("u_glowColor", glowColor[0], glowColor[1], glowColor[2]);
}

// DissolveEffect implementation
DissolveEffect::DissolveEffect() : dissolveAmount(0.0f), edgeWidth(0.1f) {
    edgeColor[0] = 1.0f;
    edgeColor[1] = 0.5f;
    edgeColor[2] = 0.0f;
}

DissolveEffect::~DissolveEffect() {}

void DissolveEffect::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    shader->setUniform("u_dissolveAmount", dissolveAmount);
    shader->setUniform("u_dissolveEdgeWidth", edgeWidth);
    shader->setUniform("u_dissolveEdgeColor", edgeColor[0], edgeColor[1], edgeColor[2]);
}

// ShaderEffectChain implementation
ShaderEffectChain::ShaderEffectChain() : enabled(true) {}

ShaderEffectChain::~ShaderEffectChain() {}

void ShaderEffectChain::addEffect(std::shared_ptr<ShaderEffect> effect) {
    if (effect) {
        effects.push_back(effect);
    }
}

void ShaderEffectChain::removeEffect(ShaderEffectType type) {
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [type](const std::shared_ptr<ShaderEffect>& effect) {
                return effect->getType() == type;
            }),
        effects.end());
}

void ShaderEffectChain::clearEffects() {
    effects.clear();
}

std::shared_ptr<ShaderEffect> ShaderEffectChain::getEffect(ShaderEffectType type) {
    for (auto& effect : effects) {
        if (effect->getType() == type) {
            return effect;
        }
    }
    return nullptr;
}

void ShaderEffectChain::apply(Shader* shader) {
    if (!shader || !enabled) return;
    
    for (auto& effect : effects) {
        if (effect && effect->isEnabled()) {
            effect->apply(shader);
        }
    }
}

// ShaderEffectLibrary implementation
ShaderEffectLibrary& ShaderEffectLibrary::getInstance() {
    static ShaderEffectLibrary instance;
    return instance;
}

void ShaderEffectLibrary::registerEffect(const std::string& name, std::shared_ptr<ShaderEffect> effect) {
    if (effect) {
        effects[name] = effect;
    }
}

void ShaderEffectLibrary::unregisterEffect(const std::string& name) {
    effects.erase(name);
}

std::shared_ptr<ShaderEffect> ShaderEffectLibrary::getEffect(const std::string& name) {
    auto it = effects.find(name);
    if (it != effects.end()) {
        return it->second;
    }
    return nullptr;
}

void ShaderEffectLibrary::clearEffects() {
    effects.clear();
}

} // namespace Graphics
} // namespace JJM
