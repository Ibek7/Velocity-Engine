#include "gameplay/StatusEffectSystem.h"
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Gameplay {

StatusEffectSystem& StatusEffectSystem::getInstance() {
    static StatusEffectSystem instance;
    return instance;
}

StatusEffectSystem::StatusEffectSystem() {
}

StatusEffectSystem::~StatusEffectSystem() {
}

void StatusEffectSystem::registerEffect(const StatusEffect& effect) {
    effectTemplates[effect.id] = effect;
}

void StatusEffectSystem::applyEffect(const std::string& effectId, const std::string& targetId) {
    auto it = effectTemplates.find(effectId);
    if (it == effectTemplates.end()) return;
    
    const StatusEffect& template_effect = it->second;
    
    auto& targetEffects = activeEffects[targetId];
    
    if (template_effect.stackable) {
        for (auto& effect : targetEffects) {
            if (effect.id == effectId) {
                if (effect.stacks < effect.maxStacks) {
                    effect.stacks++;
                    effect.remainingTime = effect.duration;
                }
                return;
            }
        }
    } else {
        for (auto& effect : targetEffects) {
            if (effect.id == effectId) {
                effect.remainingTime = effect.duration;
                return;
            }
        }
    }
    
    StatusEffect newEffect = template_effect;
    newEffect.remainingTime = newEffect.duration;
    newEffect.stacks = 1;
    newEffect.timeSinceLastTick = 0;
    
    if (newEffect.onApply) {
        newEffect.onApply();
    }
    
    targetEffects.push_back(newEffect);
    
    std::cout << "Applied effect: " << newEffect.name << " to target: " << targetId << std::endl;
}

void StatusEffectSystem::removeEffect(const std::string& targetId, const std::string& effectId) {
    auto targetIt = activeEffects.find(targetId);
    if (targetIt == activeEffects.end()) return;
    
    auto& effects = targetIt->second;
    auto effectIt = std::find_if(effects.begin(), effects.end(),
        [&effectId](const StatusEffect& e) { return e.id == effectId; });
    
    if (effectIt != effects.end()) {
        if (effectIt->onRemove) {
            effectIt->onRemove();
        }
        
        effects.erase(effectIt);
        std::cout << "Removed effect: " << effectId << " from target: " << targetId << std::endl;
    }
}

void StatusEffectSystem::clearEffects(const std::string& targetId) {
    auto it = activeEffects.find(targetId);
    if (it != activeEffects.end()) {
        for (auto& effect : it->second) {
            if (effect.onRemove) {
                effect.onRemove();
            }
        }
        it->second.clear();
    }
}

void StatusEffectSystem::update(float deltaTime) {
    for (auto& targetPair : activeEffects) {
        auto& effects = targetPair.second;
        
        for (auto& effect : effects) {
            effect.remainingTime -= deltaTime;
            effect.timeSinceLastTick += deltaTime;
            
            if (effect.onTick && effect.timeSinceLastTick >= effect.tickInterval) {
                effect.onTick(deltaTime);
                effect.timeSinceLastTick = 0;
            }
        }
        
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [](const StatusEffect& e) {
                    if (e.isExpired()) {
                        if (e.onRemove) {
                            e.onRemove();
                        }
                        return true;
                    }
                    return false;
                }),
            effects.end()
        );
    }
}

std::vector<StatusEffect*> StatusEffectSystem::getEffects(const std::string& targetId) {
    std::vector<StatusEffect*> result;
    auto it = activeEffects.find(targetId);
    if (it != activeEffects.end()) {
        for (auto& effect : it->second) {
            result.push_back(&effect);
        }
    }
    return result;
}

StatusEffect* StatusEffectSystem::getEffect(const std::string& targetId, const std::string& effectId) {
    auto it = activeEffects.find(targetId);
    if (it != activeEffects.end()) {
        for (auto& effect : it->second) {
            if (effect.id == effectId) {
                return &effect;
            }
        }
    }
    return nullptr;
}

bool StatusEffectSystem::hasEffect(const std::string& targetId, const std::string& effectId) const {
    auto it = activeEffects.find(targetId);
    if (it != activeEffects.end()) {
        for (const auto& effect : it->second) {
            if (effect.id == effectId) {
                return true;
            }
        }
    }
    return false;
}

float StatusEffectSystem::getModifier(const std::string& targetId, const std::string& modifierName) const {
    float total = 0.0f;
    auto it = activeEffects.find(targetId);
    if (it != activeEffects.end()) {
        for (const auto& effect : it->second) {
            auto modIt = effect.modifiers.find(modifierName);
            if (modIt != effect.modifiers.end()) {
                total += modIt->second * effect.stacks;
            }
        }
    }
    return total;
}

} // namespace Gameplay
} // namespace JJM
