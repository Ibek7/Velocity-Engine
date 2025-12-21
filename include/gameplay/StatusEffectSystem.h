#ifndef JJM_STATUS_EFFECT_SYSTEM_H
#define JJM_STATUS_EFFECT_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace JJM {
namespace Gameplay {

enum class EffectType { Buff, Debuff, Neutral };

struct StatusEffect {
    std::string id;
    std::string name;
    std::string description;
    EffectType type;
    
    float duration;
    float remainingTime;
    int stacks;
    int maxStacks;
    bool stackable;
    
    std::map<std::string, float> modifiers;
    
    std::function<void()> onApply;
    std::function<void()> onRemove;
    std::function<void(float)> onTick;
    float tickInterval;
    float timeSinceLastTick;
    
    bool isExpired() const { return remainingTime <= 0; }
};

class StatusEffectSystem {
public:
    static StatusEffectSystem& getInstance();
    
    void registerEffect(const StatusEffect& effect);
    void applyEffect(const std::string& effectId, const std::string& targetId);
    void removeEffect(const std::string& targetId, const std::string& effectId);
    void clearEffects(const std::string& targetId);
    
    void update(float deltaTime);
    
    std::vector<StatusEffect*> getEffects(const std::string& targetId);
    StatusEffect* getEffect(const std::string& targetId, const std::string& effectId);
    bool hasEffect(const std::string& targetId, const std::string& effectId) const;
    
    float getModifier(const std::string& targetId, const std::string& modifierName) const;

private:
    StatusEffectSystem();
    ~StatusEffectSystem();
    
    std::map<std::string, StatusEffect> effectTemplates;
    std::map<std::string, std::vector<StatusEffect>> activeEffects;
};

} // namespace Gameplay
} // namespace JJM

#endif
