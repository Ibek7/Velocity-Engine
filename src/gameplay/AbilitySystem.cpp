#include "gameplay/AbilitySystem.h"
#include <algorithm>
#include <iostream>

namespace JJM {
namespace Gameplay {

AbilitySystem& AbilitySystem::getInstance() {
    static AbilitySystem instance;
    return instance;
}

AbilitySystem::AbilitySystem() {
}

AbilitySystem::~AbilitySystem() {
}

void AbilitySystem::registerAbility(const Ability& ability) {
    abilities[ability.id] = ability;
}

void AbilitySystem::activateAbility(const std::string& abilityId, float currentTime) {
    auto it = abilities.find(abilityId);
    if (it == abilities.end()) return;
    
    Ability& ability = it->second;
    
    if (!canUseAbility(abilityId, currentTime)) {
        return;
    }
    
    ability.lastUsedTime = currentTime;
    
    if (ability.onActivate) {
        ability.onActivate();
    }
    
    if (ability.type != AbilityType::Instant) {
        activeAbilities.push_back(abilityId);
    }
    
    std::cout << "Activated ability: " << ability.name << std::endl;
}

void AbilitySystem::deactivateAbility(const std::string& abilityId) {
    auto it = abilities.find(abilityId);
    if (it == abilities.end()) return;
    
    Ability& ability = it->second;
    
    if (ability.onDeactivate) {
        ability.onDeactivate();
    }
    
    activeAbilities.erase(
        std::remove(activeAbilities.begin(), activeAbilities.end(), abilityId),
        activeAbilities.end()
    );
    
    std::cout << "Deactivated ability: " << ability.name << std::endl;
}

void AbilitySystem::update(float deltaTime, float currentTime) {
    std::vector<std::string> toDeactivate;
    
    for (const auto& abilityId : activeAbilities) {
        auto it = abilities.find(abilityId);
        if (it == abilities.end()) continue;
        
        Ability& ability = it->second;
        
        if (ability.onUpdate) {
            ability.onUpdate(deltaTime);
        }
        
        if (ability.type == AbilityType::Duration) {
            float elapsed = currentTime - ability.lastUsedTime;
            if (elapsed >= ability.duration) {
                toDeactivate.push_back(abilityId);
            }
        }
    }
    
    for (const auto& abilityId : toDeactivate) {
        deactivateAbility(abilityId);
    }
}

Ability* AbilitySystem::getAbility(const std::string& abilityId) {
    auto it = abilities.find(abilityId);
    return it != abilities.end() ? &it->second : nullptr;
}

std::vector<Ability*> AbilitySystem::getAllAbilities() {
    std::vector<Ability*> result;
    for (auto& pair : abilities) {
        result.push_back(&pair.second);
    }
    return result;
}

std::vector<Ability*> AbilitySystem::getActiveAbilities() {
    std::vector<Ability*> result;
    for (const auto& abilityId : activeAbilities) {
        auto it = abilities.find(abilityId);
        if (it != abilities.end()) {
            result.push_back(&it->second);
        }
    }
    return result;
}

bool AbilitySystem::canUseAbility(const std::string& abilityId, float currentTime) const {
    auto it = abilities.find(abilityId);
    if (it == abilities.end()) return false;
    
    const Ability& ability = it->second;
    
    if (ability.isOnCooldown(currentTime)) {
        return false;
    }
    
    if (ability.canUse && !ability.canUse()) {
        return false;
    }
    
    return true;
}

} // namespace Gameplay
} // namespace JJM
