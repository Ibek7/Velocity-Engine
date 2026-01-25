#include "gameplay/DamageSystem.h"
#include <cmath>
#include <algorithm>
#include <random>

namespace JJM {
namespace Gameplay {

// Random number generation
static std::random_device rd;
static std::mt19937 gen(rd());

static float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

// DamageResistance implementation
float DamageResistance::getResistance(DamageType type) const {
    auto it = resistances.find(type);
    return (it != resistances.end()) ? it->second : 0.0f;
}

bool DamageResistance::isImmune(DamageType type) const {
    auto it = immunities.find(type);
    return (it != immunities.end()) && it->second > 0.0f;
}

// DamageSystem implementation
DamageSystem::DamageSystem()
    : m_globalDamageMultiplier(1.0f), m_friendlyFireEnabled(false) {
}

DamageSystem::~DamageSystem() {
    shutdown();
}

void DamageSystem::initialize() {
    resetStatistics();
}

void DamageSystem::shutdown() {
    m_resistances.clear();
    m_criticalHitChances.clear();
    m_dotEffects.clear();
}

void DamageSystem::update(float deltaTime) {
    // Update all damage over time effects
    for (auto& pair : m_dotEffects) {
        Entity* entity = pair.first;
        auto& effects = pair.second;
        
        for (size_t i = 0; i < effects.size(); ) {
            DamageOverTimeEffect& effect = effects[i];
            effect.elapsed += deltaTime;
            
            if (effect.elapsed >= effect.duration) {
                // Effect expired
                effects.erase(effects.begin() + i);
            } else {
                processDamageOverTime(entity, effect, deltaTime);
                ++i;
            }
        }
    }
    
    m_stats.activeStatusEffects = 0;
    for (const auto& pair : m_dotEffects) {
        m_stats.activeStatusEffects += static_cast<int>(pair.second.size());
    }
}

float DamageSystem::dealDamage(const DamageInfo& damageInfo) {
    if (!damageInfo.attacker || !damageInfo.victim) {
        return 0.0f;
    }
    
    // Check friendly fire
    if (!m_friendlyFireEnabled) {
        // TODO: Check if attacker and victim are on same team
    }
    
    // Get victim's resistance
    const DamageResistance* resistance = getResistance(damageInfo.victim);
    DamageResistance defaultResistance;
    if (!resistance) {
        resistance = &defaultResistance;
    }
    
    // Check for immunity
    if (resistance->isImmune(damageInfo.damageType)) {
        return 0.0f;
    }
    
    // Create damage event
    DamageEvent event;
    event.info = damageInfo;
    
    // Check for dodge
    if (damageInfo.canBeDodged && rollDodge(*resistance)) {
        event.wasDodged = true;
        notifyDamageDealt(event);
        notifyDamageReceived(event);
        m_stats.dodgedAttacks++;
        return 0.0f;
    }
    
    // Check for block
    if (damageInfo.canBeBlocked && rollBlock(*resistance)) {
        event.wasBlocked = true;
        notifyDamageDealt(event);
        notifyDamageReceived(event);
        m_stats.blockedAttacks++;
        return 0.0f;
    }
    
    // Calculate final damage
    float damage = calculateDamage(damageInfo, *resistance);
    
    // Check for critical hit
    if (rollCritical(damageInfo.attacker)) {
        float critMultiplier = damageInfo.criticalMultiplier;
        // Reduce crit multiplier by victim's crit resistance
        critMultiplier = std::max(1.0f, critMultiplier - resistance->criticalResistance);
        damage *= critMultiplier;
        event.wasCritical = true;
        m_stats.criticalHits++;
    }
    
    // Apply global multiplier
    damage *= m_globalDamageMultiplier;
    
    event.damageDealt = damage;
    
    // TODO: Apply damage to entity health
    
    // Apply status effect if any
    if (damageInfo.statusEffect != StatusEffect::NONE) {
        applyStatusEffect(damageInfo.victim, damageInfo.statusEffect,
                        damageInfo.statusDuration, damageInfo.statusDamagePerTick,
                        damageInfo.attacker);
    }
    
    // Update statistics
    m_stats.totalDamageEvents++;
    m_stats.totalDamageDealt += damage;
    m_stats.damageByType[damageInfo.damageType] += damage;
    
    // Notify callbacks
    notifyDamageDealt(event);
    notifyDamageReceived(event);
    
    // Check for death
    checkDeath(damageInfo.victim, damageInfo.attacker);
    
    return damage;
}

float DamageSystem::dealDamage(Entity* attacker, Entity* victim, float damage) {
    DamageInfo info;
    info.attacker = attacker;
    info.victim = victim;
    info.baseDamage = damage;
    info.damageType = DamageType::PHYSICAL;
    info.category = DamageCategory::MELEE;
    
    return dealDamage(info);
}

int DamageSystem::dealAreaDamage(const float center[3], float radius,
                                const DamageInfo& damageInfo, float falloff) {
    // TODO: Query all entities in radius
    // For now, return 0
    int count = 0;
    
    // Example implementation (would need spatial query system):
    // auto entities = spatialQuery(center, radius);
    // for (auto* entity : entities) {
    //     float distance = calculateDistance(center, entity->getPosition());
    //     float distanceFactor = 1.0f - (distance / radius) * falloff;
    //     
    //     DamageInfo info = damageInfo;
    //     info.victim = entity;
    //     info.baseDamage *= distanceFactor;
    //     
    //     dealDamage(info);
    //     count++;
    // }
    
    return count;
}

void DamageSystem::setResistance(Entity* entity, const DamageResistance& resistance) {
    m_resistances[entity] = resistance;
}

const DamageResistance* DamageSystem::getResistance(Entity* entity) const {
    auto it = m_resistances.find(entity);
    return (it != m_resistances.end()) ? &it->second : nullptr;
}

void DamageSystem::addResistance(Entity* entity, DamageType type, float amount) {
    auto& resistance = m_resistances[entity];
    resistance.resistances[type] = std::min(1.0f,
        resistance.resistances[type] + amount);
}

void DamageSystem::removeResistance(Entity* entity, DamageType type, float amount) {
    auto it = m_resistances.find(entity);
    if (it != m_resistances.end()) {
        auto& resistance = it->second;
        resistance.resistances[type] = std::max(0.0f,
            resistance.resistances[type] - amount);
    }
}

void DamageSystem::applyStatusEffect(Entity* entity, StatusEffect effect, float duration,
                                    float damagePerTick, Entity* source) {
    DamageOverTimeEffect dotEffect;
    dotEffect.type = effect;
    dotEffect.source = source;
    dotEffect.damagePerTick = damagePerTick;
    dotEffect.tickInterval = 1.0f;  // Default 1 tick per second
    dotEffect.duration = duration;
    dotEffect.elapsed = 0.0f;
    dotEffect.timeSinceLastTick = 0.0f;
    
    // Determine damage type based on status effect
    switch (effect) {
        case StatusEffect::BURNING:
            dotEffect.damageType = DamageType::FIRE;
            break;
        case StatusEffect::POISONED:
            dotEffect.damageType = DamageType::POISON;
            break;
        case StatusEffect::BLEEDING:
            dotEffect.damageType = DamageType::PHYSICAL;
            break;
        default:
            dotEffect.damageType = DamageType::TRUE;
            break;
    }
    
    m_dotEffects[entity].push_back(dotEffect);
}

void DamageSystem::removeStatusEffect(Entity* entity, StatusEffect effect) {
    auto it = m_dotEffects.find(entity);
    if (it != m_dotEffects.end()) {
        auto& effects = it->second;
        effects.erase(
            std::remove_if(effects.begin(), effects.end(),
                [effect](const DamageOverTimeEffect& e) { return e.type == effect; }),
            effects.end()
        );
    }
}

bool DamageSystem::hasStatusEffect(Entity* entity, StatusEffect effect) const {
    auto it = m_dotEffects.find(entity);
    if (it != m_dotEffects.end()) {
        for (const auto& dotEffect : it->second) {
            if (dotEffect.type == effect) {
                return true;
            }
        }
    }
    return false;
}

std::vector<DamageOverTimeEffect> DamageSystem::getActiveEffects(Entity* entity) const {
    auto it = m_dotEffects.find(entity);
    return (it != m_dotEffects.end()) ? it->second : std::vector<DamageOverTimeEffect>();
}

void DamageSystem::setDamageDealtCallback(std::function<void(const DamageEvent&)> callback) {
    m_damageDealtCallback = callback;
}

void DamageSystem::setDamageReceivedCallback(std::function<void(const DamageEvent&)> callback) {
    m_damageReceivedCallback = callback;
}

void DamageSystem::setDeathCallback(std::function<void(Entity*, Entity*)> callback) {
    m_deathCallback = callback;
}

void DamageSystem::setCriticalHitChance(Entity* entity, float chance) {
    m_criticalHitChances[entity] = std::max(0.0f, std::min(1.0f, chance));
}

float DamageSystem::getCriticalHitChance(Entity* entity) const {
    auto it = m_criticalHitChances.find(entity);
    return (it != m_criticalHitChances.end()) ? it->second : 0.0f;
}

DamageSystem::Statistics DamageSystem::getStatistics() const {
    return m_stats;
}

void DamageSystem::resetStatistics() {
    m_stats = Statistics();
}

float DamageSystem::calculateDamage(const DamageInfo& info,
                                   const DamageResistance& resistance) {
    float damage = info.baseDamage;
    
    // Apply armor if physical damage and not penetrating
    if (info.damageType == DamageType::PHYSICAL && !info.penetratesArmor) {
        damage = applyArmor(damage, resistance.armorValue);
    }
    
    // Apply resistance (except for TRUE damage)
    if (info.damageType != DamageType::TRUE) {
        damage = applyResistance(damage, resistance, info.damageType);
    }
    
    return std::max(0.0f, damage);
}

bool DamageSystem::rollCritical(Entity* attacker) const {
    float chance = getCriticalHitChance(attacker);
    return randomFloat(0.0f, 1.0f) < chance;
}

bool DamageSystem::rollBlock(const DamageResistance& resistance) const {
    return randomFloat(0.0f, 1.0f) < resistance.blockChance;
}

bool DamageSystem::rollDodge(const DamageResistance& resistance) const {
    return randomFloat(0.0f, 1.0f) < resistance.dodgeChance;
}

float DamageSystem::applyArmor(float damage, float armor) const {
    // Simple armor formula: damage * (100 / (100 + armor))
    if (armor <= 0) return damage;
    float reduction = 100.0f / (100.0f + armor);
    return damage * reduction;
}

float DamageSystem::applyResistance(float damage, const DamageResistance& resistance,
                                   DamageType type) const {
    float resistanceValue = resistance.getResistance(type);
    return damage * (1.0f - resistanceValue);
}

void DamageSystem::processDamageOverTime(Entity* entity, DamageOverTimeEffect& effect,
                                        float deltaTime) {
    effect.timeSinceLastTick += deltaTime;
    
    if (effect.timeSinceLastTick >= effect.tickInterval) {
        // Deal damage tick
        DamageInfo info;
        info.attacker = effect.source;
        info.victim = entity;
        info.baseDamage = effect.damagePerTick;
        info.damageType = effect.damageType;
        info.category = DamageCategory::OVER_TIME;
        info.canBeBlocked = false;
        info.canBeDodged = false;
        
        dealDamage(info);
        
        effect.timeSinceLastTick = 0.0f;
    }
}

void DamageSystem::notifyDamageDealt(const DamageEvent& event) {
    if (m_damageDealtCallback) {
        m_damageDealtCallback(event);
    }
}

void DamageSystem::notifyDamageReceived(const DamageEvent& event) {
    if (m_damageReceivedCallback) {
        m_damageReceivedCallback(event);
    }
}

void DamageSystem::checkDeath(Entity* victim, Entity* attacker) {
    // TODO: Check entity health
    // if (victim->getHealth() <= 0) {
    //     if (m_deathCallback) {
    //         m_deathCallback(victim, attacker);
    //     }
    // }
}

// Helper functions
namespace DamageHelpers {
    const char* getDamageTypeName(DamageType type) {
        switch (type) {
            case DamageType::PHYSICAL: return "Physical";
            case DamageType::FIRE: return "Fire";
            case DamageType::ICE: return "Ice";
            case DamageType::ELECTRIC: return "Electric";
            case DamageType::POISON: return "Poison";
            case DamageType::RADIATION: return "Radiation";
            case DamageType::EXPLOSIVE: return "Explosive";
            case DamageType::ENERGY: return "Energy";
            case DamageType::HOLY: return "Holy";
            case DamageType::DARK: return "Dark";
            case DamageType::PSYCHIC: return "Psychic";
            case DamageType::TRUE: return "True";
            default: return "Unknown";
        }
    }
    
    const char* getStatusEffectName(StatusEffect effect) {
        switch (effect) {
            case StatusEffect::NONE: return "None";
            case StatusEffect::BURNING: return "Burning";
            case StatusEffect::FROZEN: return "Frozen";
            case StatusEffect::SHOCKED: return "Shocked";
            case StatusEffect::POISONED: return "Poisoned";
            case StatusEffect::BLEEDING: return "Bleeding";
            case StatusEffect::WEAKENED: return "Weakened";
            case StatusEffect::VULNERABLE: return "Vulnerable";
            case StatusEffect::STUNNED: return "Stunned";
            case StatusEffect::SLOWED: return "Slowed";
            default: return "Unknown";
        }
    }
    
    DamageResistance getDefaultResistance() {
        DamageResistance resistance;
        resistance.armorValue = 0.0f;
        resistance.blockChance = 0.0f;
        resistance.dodgeChance = 0.0f;
        resistance.criticalResistance = 0.0f;
        return resistance;
    }
}

} // namespace Gameplay
} // namespace JJM
