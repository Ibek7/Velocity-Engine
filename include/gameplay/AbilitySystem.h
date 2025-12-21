#ifndef JJM_ABILITY_SYSTEM_H
#define JJM_ABILITY_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <chrono>

namespace JJM {
namespace Gameplay {

enum class AbilityType { Instant, Duration, Channeled };

struct AbilityCost {
    std::map<std::string, int> resources;
};

struct Ability {
    std::string id;
    std::string name;
    std::string description;
    AbilityType type;
    
    float cooldownTime;
    float lastUsedTime;
    float duration;
    
    AbilityCost cost;
    std::function<bool()> canUse;
    std::function<void()> onActivate;
    std::function<void()> onDeactivate;
    std::function<void(float)> onUpdate;
    
    bool isOnCooldown(float currentTime) const {
        return (currentTime - lastUsedTime) < cooldownTime;
    }
    
    float getCooldownRemaining(float currentTime) const {
        float elapsed = currentTime - lastUsedTime;
        return elapsed < cooldownTime ? cooldownTime - elapsed : 0.0f;
    }
};

class AbilitySystem {
public:
    static AbilitySystem& getInstance();
    
    void registerAbility(const Ability& ability);
    void activateAbility(const std::string& abilityId, float currentTime);
    void deactivateAbility(const std::string& abilityId);
    
    void update(float deltaTime, float currentTime);
    
    Ability* getAbility(const std::string& abilityId);
    std::vector<Ability*> getAllAbilities();
    std::vector<Ability*> getActiveAbilities();
    
    bool canUseAbility(const std::string& abilityId, float currentTime) const;

private:
    AbilitySystem();
    ~AbilitySystem();
    
    std::map<std::string, Ability> abilities;
    std::vector<std::string> activeAbilities;
};

} // namespace Gameplay
} // namespace JJM

#endif
