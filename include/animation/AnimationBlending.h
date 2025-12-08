#pragma once

#include "animation/Animation.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace JJM {
namespace Animation {

struct BlendNode {
    std::string animationName;
    float weight;
    float speed;
    
    BlendNode() : weight(0.0f), speed(1.0f) {}
    BlendNode(const std::string& name, float weight = 1.0f)
        : animationName(name), weight(weight), speed(1.0f) {}
};

class AnimationBlender {
public:
    AnimationBlender();
    ~AnimationBlender();
    
    void addAnimation(const std::string& name, std::shared_ptr<AnimationClip> clip);
    void removeAnimation(const std::string& name);
    
    void setWeight(const std::string& name, float weight);
    float getWeight(const std::string& name) const;
    
    void blend(float deltaTime);
    
    void normalizeWeights();
    
    void clearBlend();
    
    const std::vector<BlendNode>& getBlendNodes() const { return blendNodes; }

private:
    std::unordered_map<std::string, std::shared_ptr<AnimationClip>> animations;
    std::vector<BlendNode> blendNodes;
    
    void updateBlendNodes();
};

class BlendTree {
public:
    BlendTree();
    ~BlendTree();
    
    void setBlendParameter(const std::string& name, float value);
    float getBlendParameter(const std::string& name) const;
    
    void addBlendSpace1D(const std::string& paramName,
                        const std::vector<std::pair<float, std::string>>& animations);
    
    void addBlendSpace2D(const std::string& paramX, const std::string& paramY,
                        const std::vector<std::tuple<float, float, std::string>>& animations);
    
    void update(float deltaTime);
    
    std::vector<BlendNode> getBlendedAnimations() const;

private:
    std::unordered_map<std::string, float> parameters;
    
    struct BlendSpace1D {
        std::string paramName;
        std::vector<std::pair<float, std::string>> animations;
    };
    
    struct BlendSpace2D {
        std::string paramX;
        std::string paramY;
        std::vector<std::tuple<float, float, std::string>> animations;
    };
    
    std::vector<BlendSpace1D> blendSpaces1D;
    std::vector<BlendSpace2D> blendSpaces2D;
    
    std::vector<BlendNode> calculateBlend1D(const BlendSpace1D& space) const;
    std::vector<BlendNode> calculateBlend2D(const BlendSpace2D& space) const;
};

class CrossfadeController {
public:
    CrossfadeController();
    
    void startCrossfade(const std::string& fromAnim, const std::string& toAnim, float duration);
    
    void update(float deltaTime);
    
    bool isCrossfading() const { return crossfading; }
    float getProgress() const { return progress; }
    
    std::vector<BlendNode> getCurrentBlend() const;

private:
    bool crossfading;
    std::string fromAnimation;
    std::string toAnimation;
    float duration;
    float progress;
};

class LayeredAnimationBlender {
public:
    LayeredAnimationBlender();
    ~LayeredAnimationBlender();
    
    void addLayer(const std::string& layerName, float weight = 1.0f);
    void removeLayer(const std::string& layerName);
    
    void setLayerWeight(const std::string& layerName, float weight);
    float getLayerWeight(const std::string& layerName) const;
    
    void setLayerAnimation(const std::string& layerName, const std::string& animationName);
    
    void setLayerMask(const std::string& layerName, const std::vector<std::string>& boneMask);
    
    void update(float deltaTime);
    
    std::vector<BlendNode> getFinalBlend() const;

private:
    struct AnimationLayer {
        std::string name;
        std::string currentAnimation;
        float weight;
        std::vector<std::string> boneMask;
        
        AnimationLayer() : weight(1.0f) {}
    };
    
    std::vector<AnimationLayer> layers;
    
    AnimationLayer* findLayer(const std::string& name);
};

class AdditiveBlending {
public:
    AdditiveBlending();
    
    void setBaseAnimation(const std::string& animName);
    void addAdditiveAnimation(const std::string& animName, float weight);
    
    void removeAdditiveAnimation(const std::string& animName);
    
    void update(float deltaTime);
    
    std::vector<BlendNode> getBlendedResult() const;

private:
    std::string baseAnimation;
    std::vector<BlendNode> additiveAnimations;
};

} // namespace Animation
} // namespace JJM
