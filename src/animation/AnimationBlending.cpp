#include "animation/AnimationBlending.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Animation {

AnimationBlender::AnimationBlender() {}

AnimationBlender::~AnimationBlender() {}

void AnimationBlender::addAnimation(const std::string& name, std::shared_ptr<AnimationClip> clip) {
    animations[name] = clip;
    updateBlendNodes();
}

void AnimationBlender::removeAnimation(const std::string& name) {
    animations.erase(name);
    updateBlendNodes();
}

void AnimationBlender::setWeight(const std::string& name, float weight) {
    for (auto& node : blendNodes) {
        if (node.animationName == name) {
            node.weight = weight;
            return;
        }
    }
}

float AnimationBlender::getWeight(const std::string& name) const {
    for (const auto& node : blendNodes) {
        if (node.animationName == name) {
            return node.weight;
        }
    }
    return 0.0f;
}

void AnimationBlender::blend(float deltaTime) {
    (void)deltaTime;
}

void AnimationBlender::normalizeWeights() {
    float totalWeight = 0.0f;
    for (const auto& node : blendNodes) {
        totalWeight += node.weight;
    }
    
    if (totalWeight > 0.0f) {
        for (auto& node : blendNodes) {
            node.weight /= totalWeight;
        }
    }
}

void AnimationBlender::clearBlend() {
    blendNodes.clear();
}

void AnimationBlender::updateBlendNodes() {
    blendNodes.clear();
    for (const auto& pair : animations) {
        blendNodes.emplace_back(pair.first, 0.0f);
    }
}

BlendTree::BlendTree() {}

BlendTree::~BlendTree() {}

void BlendTree::setBlendParameter(const std::string& name, float value) {
    parameters[name] = value;
}

float BlendTree::getBlendParameter(const std::string& name) const {
    auto it = parameters.find(name);
    return (it != parameters.end()) ? it->second : 0.0f;
}

void BlendTree::addBlendSpace1D(const std::string& paramName,
                               const std::vector<std::pair<float, std::string>>& animations) {
    BlendSpace1D space;
    space.paramName = paramName;
    space.animations = animations;
    blendSpaces1D.push_back(space);
}

void BlendTree::addBlendSpace2D(const std::string& paramX, const std::string& paramY,
                               const std::vector<std::tuple<float, float, std::string>>& animations) {
    BlendSpace2D space;
    space.paramX = paramX;
    space.paramY = paramY;
    space.animations = animations;
    blendSpaces2D.push_back(space);
}

void BlendTree::update(float deltaTime) {
    (void)deltaTime;
}

std::vector<BlendNode> BlendTree::getBlendedAnimations() const {
    std::vector<BlendNode> result;
    
    for (const auto& space : blendSpaces1D) {
        auto blend = calculateBlend1D(space);
        result.insert(result.end(), blend.begin(), blend.end());
    }
    
    for (const auto& space : blendSpaces2D) {
        auto blend = calculateBlend2D(space);
        result.insert(result.end(), blend.begin(), blend.end());
    }
    
    return result;
}

std::vector<BlendNode> BlendTree::calculateBlend1D(const BlendSpace1D& space) const {
    std::vector<BlendNode> result;
    float paramValue = getBlendParameter(space.paramName);
    
    for (size_t i = 0; i < space.animations.size(); ++i) {
        float animValue = space.animations[i].first;
        float distance = std::abs(paramValue - animValue);
        float weight = 1.0f / (1.0f + distance);
        
        result.emplace_back(space.animations[i].second, weight);
    }
    
    return result;
}

std::vector<BlendNode> BlendTree::calculateBlend2D(const BlendSpace2D& space) const {
    std::vector<BlendNode> result;
    float paramX = getBlendParameter(space.paramX);
    float paramY = getBlendParameter(space.paramY);
    
    for (size_t i = 0; i < space.animations.size(); ++i) {
        float animX = std::get<0>(space.animations[i]);
        float animY = std::get<1>(space.animations[i]);
        
        float dx = paramX - animX;
        float dy = paramY - animY;
        float distance = std::sqrt(dx * dx + dy * dy);
        float weight = 1.0f / (1.0f + distance);
        
        result.emplace_back(std::get<2>(space.animations[i]), weight);
    }
    
    return result;
}

CrossfadeController::CrossfadeController()
    : crossfading(false), duration(0.0f), progress(0.0f) {}

void CrossfadeController::startCrossfade(const std::string& fromAnim, const std::string& toAnim, float duration) {
    this->fromAnimation = fromAnim;
    this->toAnimation = toAnim;
    this->duration = duration;
    this->progress = 0.0f;
    this->crossfading = true;
}

void CrossfadeController::update(float deltaTime) {
    if (!crossfading) return;
    
    progress += deltaTime / duration;
    if (progress >= 1.0f) {
        progress = 1.0f;
        crossfading = false;
    }
}

std::vector<BlendNode> CrossfadeController::getCurrentBlend() const {
    std::vector<BlendNode> result;
    
    if (crossfading) {
        result.emplace_back(fromAnimation, 1.0f - progress);
        result.emplace_back(toAnimation, progress);
    }
    
    return result;
}

LayeredAnimationBlender::LayeredAnimationBlender() {}

LayeredAnimationBlender::~LayeredAnimationBlender() {}

void LayeredAnimationBlender::addLayer(const std::string& layerName, float weight) {
    AnimationLayer layer;
    layer.name = layerName;
    layer.weight = weight;
    layers.push_back(layer);
}

void LayeredAnimationBlender::removeLayer(const std::string& layerName) {
    layers.erase(
        std::remove_if(layers.begin(), layers.end(),
            [&layerName](const AnimationLayer& layer) {
                return layer.name == layerName;
            }),
        layers.end()
    );
}

void LayeredAnimationBlender::setLayerWeight(const std::string& layerName, float weight) {
    AnimationLayer* layer = findLayer(layerName);
    if (layer) {
        layer->weight = weight;
    }
}

float LayeredAnimationBlender::getLayerWeight(const std::string& layerName) const {
    for (const auto& layer : layers) {
        if (layer.name == layerName) {
            return layer.weight;
        }
    }
    return 0.0f;
}

void LayeredAnimationBlender::setLayerAnimation(const std::string& layerName, const std::string& animationName) {
    AnimationLayer* layer = findLayer(layerName);
    if (layer) {
        layer->currentAnimation = animationName;
    }
}

void LayeredAnimationBlender::setLayerMask(const std::string& layerName, const std::vector<std::string>& boneMask) {
    AnimationLayer* layer = findLayer(layerName);
    if (layer) {
        layer->boneMask = boneMask;
    }
}

void LayeredAnimationBlender::update(float deltaTime) {
    (void)deltaTime;
}

std::vector<BlendNode> LayeredAnimationBlender::getFinalBlend() const {
    std::vector<BlendNode> result;
    
    for (const auto& layer : layers) {
        if (!layer.currentAnimation.empty()) {
            result.emplace_back(layer.currentAnimation, layer.weight);
        }
    }
    
    return result;
}

LayeredAnimationBlender::AnimationLayer* LayeredAnimationBlender::findLayer(const std::string& name) {
    for (auto& layer : layers) {
        if (layer.name == name) {
            return &layer;
        }
    }
    return nullptr;
}

AdditiveBlending::AdditiveBlending() {}

void AdditiveBlending::setBaseAnimation(const std::string& animName) {
    baseAnimation = animName;
}

void AdditiveBlending::addAdditiveAnimation(const std::string& animName, float weight) {
    additiveAnimations.emplace_back(animName, weight);
}

void AdditiveBlending::removeAdditiveAnimation(const std::string& animName) {
    additiveAnimations.erase(
        std::remove_if(additiveAnimations.begin(), additiveAnimations.end(),
            [&animName](const BlendNode& node) {
                return node.animationName == animName;
            }),
        additiveAnimations.end()
    );
}

void AdditiveBlending::update(float deltaTime) {
    (void)deltaTime;
}

std::vector<BlendNode> AdditiveBlending::getBlendedResult() const {
    std::vector<BlendNode> result;
    
    if (!baseAnimation.empty()) {
        result.emplace_back(baseAnimation, 1.0f);
    }
    
    for (const auto& additive : additiveAnimations) {
        result.push_back(additive);
    }
    
    return result;
}

} // namespace Animation
} // namespace JJM
