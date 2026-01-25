#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace JJM {

// Forward declarations
class Entity;

namespace Gameplay {

/**
 * Types of damage that can be dealt
 */
enum class DamageType {
    PHYSICAL,      // Standard physical damage
    FIRE,          // Fire/heat damage
    ICE,           // Cold/frost damage
    ELECTRIC,      // Electrical/lightning damage
    POISON,        // Poison/toxic damage
    RADIATION,     // Radiation damage
    EXPLOSIVE,     // Explosion damage
    ENERGY,        // Energy/plasma damage
    HOLY,          // Holy/divine damage
    DARK,          // Dark/shadow damage
    PSYCHIC,       // Mental/psychic damage
    TRUE          // True damage (ignores all resistances)
};

/**
 * Damage categories for different gameplay purposes
 */
enum class DamageCategory {
    MELEE,         // Close-range attack
    RANGED,        // Projectile/ranged attack
    AREA,          // Area of effect
    OVER_TIME,     // Damage over time (DOT)
    ENVIRONMENTAL, // Environmental hazard
    SELF           // Self-inflicted damage
};

/**
 * Status effects that can be applied with damage
 */
enum class StatusEffect {
    NONE,
    BURNING,       // Damage over time from fire
    FROZEN,        // Slowed or immobilized
    SHOCKED,       // Stunned or incapacitated
    POISONED,      // Damage over time from poison
    BLEEDING,      // Physical damage over time
    WEAKENED,      // Reduced damage output
    VULNERABLE,    // Increased damage taken
    STUNNED,       // Cannot act
    SLOWED         // Reduced movement speed
};

/**
 * Information about a damage event
 */
struct DamageInfo {
    Entity* attacker = nullptr;           // Entity causing damage
    Entity* victim = nullptr;             // Entity receiving damage
    
    DamageType damageType = DamageType::PHYSICAL;
    DamageCategory category = DamageCategory::MELEE;
    
    float baseDamage = 0.0f;             // Base damage amount
    float finalDamage = 0.0f;            // Final damage after modifiers
    
    bool isCritical = false;              // Critical hit
    float criticalMultiplier = 2.0f;      // Multiplier for critical hits
    
    StatusEffect statusEffect = StatusEffect::NONE;
    float statusDuration = 0.0f;          // Duration of status effect
    float statusDamagePerTick = 0.0f;     // Damage per tick for DOT effects
    
    float position[3] = {0, 0, 0};        // World position of damage
    std::string weaponId;                 // ID of weapon used
    std::string abilityId;                // ID of ability used
    
    bool canBeBlocked = true;             // Can be blocked by shields
    bool canBeDodged = true;              // Can be dodged
    bool penetratesArmor = false;         // Ignores armor
};

/**
 * Damage resistance configuration for an entity
 */
struct DamageResistance {
    std::unordered_map<DamageType, float> resistances;  // Resistance % by type (0-1)
    std::unordered_map<DamageType, float> immunities;   // Immunity flags
    
    float armorValue = 0.0f;              // Physical armor
    float blockChance = 0.0f;             // Chance to block (0-1)
    float dodgeChance = 0.0f;             // Chance to dodge (0-1)
    float criticalResistance = 0.0f;      // Reduces critical damage %
    
    /**
     * Get resistance value for a damage type
     * @param type Damage type
     * @return Resistance value (0 = no resistance, 1 = full immunity)
     */
    float getResistance(DamageType type) const;
    
    /**
     * Check if immune to a damage type
     * @param type Damage type
     * @return True if immune
     */
    bool isImmune(DamageType type) const;
};

/**
 * Active damage over time effect on an entity
 */
struct DamageOverTimeEffect {
    StatusEffect type;
    Entity* source;                       // Entity that applied the effect
    float damagePerTick;
    float tickInterval;                   // Time between damage ticks
    float duration;                       // Total duration
    float elapsed;                        // Time elapsed
    float timeSinceLastTick;
    DamageType damageType;
};

/**
 * Damage event for callbacks
 */
struct DamageEvent {
    DamageInfo info;
    bool wasBlocked = false;
    bool wasDodged = false;
    bool wasCritical = false;
    float damageDealt = 0.0f;
};

/**
 * System for handling damage calculation, resistances, and effects
 */
class DamageSystem {
public:
    DamageSystem();
    ~DamageSystem();
    
    /**
     * Initialize the damage system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update damage over time effects
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Damage dealing
    
    /**
     * Deal damage to an entity
     * @param damageInfo Information about the damage event
     * @return Final damage dealt after all calculations
     */
    float dealDamage(const DamageInfo& damageInfo);
    
    /**
     * Simple damage dealing (physical damage)
     * @param attacker Entity dealing damage
     * @param victim Entity receiving damage
     * @param damage Amount of damage
     * @return Final damage dealt
     */
    float dealDamage(Entity* attacker, Entity* victim, float damage);
    
    /**
     * Deal area damage to all entities in radius
     * @param center Center position [x, y, z]
     * @param radius Damage radius
     * @param damageInfo Base damage info (will be applied to all in radius)
     * @param falloff Damage falloff (0 = no falloff, 1 = linear falloff)
     * @return Number of entities damaged
     */
    int dealAreaDamage(const float center[3], float radius,
                      const DamageInfo& damageInfo, float falloff = 0.5f);
    
    // Resistance management
    
    /**
     * Set damage resistance for an entity
     * @param entity Entity to configure
     * @param resistance Resistance configuration
     */
    void setResistance(Entity* entity, const DamageResistance& resistance);
    
    /**
     * Get damage resistance for an entity
     * @param entity Entity to query
     * @return Resistance configuration
     */
    const DamageResistance* getResistance(Entity* entity) const;
    
    /**
     * Add resistance to a specific damage type
     * @param entity Entity to modify
     * @param type Damage type
     * @param amount Resistance amount (0-1)
     */
    void addResistance(Entity* entity, DamageType type, float amount);
    
    /**
     * Remove resistance to a specific damage type
     * @param entity Entity to modify
     * @param type Damage type
     * @param amount Resistance amount to remove
     */
    void removeResistance(Entity* entity, DamageType type, float amount);
    
    // Status effects and DOT
    
    /**
     * Apply a status effect to an entity
     * @param entity Entity to affect
     * @param effect Status effect type
     * @param duration Duration in seconds
     * @param damagePerTick Damage per tick (for DOT effects)
     * @param source Entity that applied the effect
     */
    void applyStatusEffect(Entity* entity, StatusEffect effect, float duration,
                          float damagePerTick = 0.0f, Entity* source = nullptr);
    
    /**
     * Remove a status effect from an entity
     * @param entity Entity to modify
     * @param effect Status effect to remove
     */
    void removeStatusEffect(Entity* entity, StatusEffect effect);
    
    /**
     * Check if entity has a status effect
     * @param entity Entity to check
     * @param effect Status effect type
     * @return True if entity has the effect
     */
    bool hasStatusEffect(Entity* entity, StatusEffect effect) const;
    
    /**
     * Get all active status effects on an entity
     * @param entity Entity to query
     * @return Vector of active DOT effects
     */
    std::vector<DamageOverTimeEffect> getActiveEffects(Entity* entity) const;
    
    // Callbacks
    
    /**
     * Set callback for damage dealt events
     * @param callback Function to call when damage is dealt
     */
    void setDamageDealtCallback(std::function<void(const DamageEvent&)> callback);
    
    /**
     * Set callback for damage received events
     * @param callback Function to call when entity receives damage
     */
    void setDamageReceivedCallback(std::function<void(const DamageEvent&)> callback);
    
    /**
     * Set callback for entity death
     * @param callback Function to call when entity dies from damage
     */
    void setDeathCallback(std::function<void(Entity*, Entity*)> callback);
    
    // Configuration
    
    /**
     * Set global damage multiplier
     * @param multiplier Damage multiplier (1.0 = normal)
     */
    void setGlobalDamageMultiplier(float multiplier) { m_globalDamageMultiplier = multiplier; }
    
    /**
     * Get global damage multiplier
     * @return Current multiplier
     */
    float getGlobalDamageMultiplier() const { return m_globalDamageMultiplier; }
    
    /**
     * Enable or disable friendly fire
     * @param enabled True to enable
     */
    void setFriendlyFire(bool enabled) { m_friendlyFireEnabled = enabled; }
    
    /**
     * Check if friendly fire is enabled
     * @return True if enabled
     */
    bool isFriendlyFireEnabled() const { return m_friendlyFireEnabled; }
    
    /**
     * Set critical hit chance
     * @param entity Entity to configure
     * @param chance Critical hit chance (0-1)
     */
    void setCriticalHitChance(Entity* entity, float chance);
    
    /**
     * Get critical hit chance for an entity
     * @param entity Entity to query
     * @return Critical hit chance (0-1)
     */
    float getCriticalHitChance(Entity* entity) const;
    
    // Statistics
    
    /**
     * Statistics about damage system
     */
    struct Statistics {
        int totalDamageEvents = 0;
        float totalDamageDealt = 0.0f;
        int criticalHits = 0;
        int blockedAttacks = 0;
        int dodgedAttacks = 0;
        int activeStatusEffects = 0;
        
        std::unordered_map<DamageType, float> damageByType;
    };
    
    /**
     * Get damage statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
    /**
     * Reset statistics
     */
    void resetStatistics();

private:
    // Entity resistance data
    std::unordered_map<Entity*, DamageResistance> m_resistances;
    
    // Entity critical hit chances
    std::unordered_map<Entity*, float> m_criticalHitChances;
    
    // Active damage over time effects
    std::unordered_map<Entity*, std::vector<DamageOverTimeEffect>> m_dotEffects;
    
    // Callbacks
    std::function<void(const DamageEvent&)> m_damageDealtCallback;
    std::function<void(const DamageEvent&)> m_damageReceivedCallback;
    std::function<void(Entity*, Entity*)> m_deathCallback;
    
    // Configuration
    float m_globalDamageMultiplier;
    bool m_friendlyFireEnabled;
    
    // Statistics
    Statistics m_stats;
    
    // Internal methods
    float calculateDamage(const DamageInfo& info, const DamageResistance& resistance);
    bool rollCritical(Entity* attacker) const;
    bool rollBlock(const DamageResistance& resistance) const;
    bool rollDodge(const DamageResistance& resistance) const;
    float applyArmor(float damage, float armor) const;
    float applyResistance(float damage, const DamageResistance& resistance, DamageType type) const;
    void processDamageOverTime(Entity* entity, DamageOverTimeEffect& effect, float deltaTime);
    void notifyDamageDealt(const DamageEvent& event);
    void notifyDamageReceived(const DamageEvent& event);
    void checkDeath(Entity* victim, Entity* attacker);
};

// Helper functions
namespace DamageHelpers {
    /**
     * Get string name for damage type
     * @param type Damage type
     * @return String name
     */
    const char* getDamageTypeName(DamageType type);
    
    /**
     * Get string name for status effect
     * @param effect Status effect
     * @return String name
     */
    const char* getStatusEffectName(StatusEffect effect);
    
    /**
     * Get default resistance configuration (no resistances)
     * @return Empty resistance configuration
     */
    DamageResistance getDefaultResistance();
}

} // namespace Gameplay
} // namespace JJM
