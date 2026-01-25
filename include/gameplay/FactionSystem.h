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
 * Relationship status between factions
 */
enum class FactionRelationship {
    ALLIED,        // Friendly, will help in combat
    FRIENDLY,      // Positive but not allied
    NEUTRAL,       // No strong feelings
    UNFRIENDLY,    // Negative but not hostile
    HOSTILE,       // Will attack on sight
    ENEMY          // At war
};

/**
 * Faction data
 */
struct Faction {
    std::string id;
    std::string name;
    std::string description;
    std::string iconPath;
    
    // Visual identity
    float color[4] = {1, 1, 1, 1};     // RGBA color
    std::string emblemPath;
    
    // Properties
    bool isPlayerFaction = false;
    bool canJoin = true;                // Can player join this faction
    bool hidden = false;                // Hidden until discovered
    
    // Leadership
    std::string leaderName;
    Entity* leaderEntity = nullptr;
    
    // Locations
    std::vector<std::string> territories;  // Territory IDs controlled
    std::string headquartersLocation;
};

/**
 * Reputation level with a faction
 */
enum class ReputationLevel {
    REVERED,       // Maximum positive reputation
    EXALTED,       // Very high reputation
    HONORED,       // High reputation
    FRIENDLY,      // Positive reputation
    NEUTRAL,       // Starting reputation
    UNFRIENDLY,    // Negative reputation
    HOSTILE,       // Very negative reputation
    HATED          // Maximum negative reputation
};

/**
 * Player's reputation with a faction
 */
struct FactionReputation {
    std::string factionId;
    int reputationValue = 0;           // -1000 to +1000
    ReputationLevel level = ReputationLevel::NEUTRAL;
    
    bool atWarWith = false;
    bool isMember = false;
    int memberRank = 0;                // 0 = not member, higher = better rank
    
    // Tracking
    int questsCompleted = 0;
    int questsFailed = 0;
    int killsAgainst = 0;              // Kills against this faction
    int killsFor = 0;                  // Kills helping this faction
};

/**
 * Relationship between two factions
 */
struct FactionRelationshipData {
    std::string faction1Id;
    std::string faction2Id;
    FactionRelationship relationship;
    int relationshipValue = 0;         // -100 to +100
    
    bool atWar = false;
    std::vector<std::string> sharedEnemies;
    std::vector<std::string> sharedAllies;
};

/**
 * Reputation change event
 */
struct ReputationEvent {
    std::string factionId;
    int oldValue;
    int newValue;
    ReputationLevel oldLevel;
    ReputationLevel newLevel;
    std::string reason;
};

/**
 * System for managing factions and reputation
 */
class FactionSystem {
public:
    FactionSystem();
    ~FactionSystem();
    
    /**
     * Initialize the faction system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update faction relationships and reputation decay
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Faction management
    
    /**
     * Register a faction
     * @param faction Faction to register
     * @return True if registered successfully
     */
    bool registerFaction(const Faction& faction);
    
    /**
     * Load factions from file
     * @param filepath Path to faction definitions file
     * @return Number of factions loaded
     */
    int loadFactions(const std::string& filepath);
    
    /**
     * Get faction by ID
     * @param id Faction identifier
     * @return Pointer to faction, or nullptr if not found
     */
    Faction* getFaction(const std::string& id);
    
    /**
     * Get all factions
     * @return Vector of all factions
     */
    std::vector<Faction*> getAllFactions();
    
    /**
     * Get all discovered (non-hidden) factions
     * @return Vector of discovered factions
     */
    std::vector<Faction*> getDiscoveredFactions();
    
    /**
     * Discover a hidden faction
     * @param id Faction identifier
     */
    void discoverFaction(const std::string& id);
    
    // Reputation management
    
    /**
     * Get player's reputation with a faction
     * @param factionId Faction identifier
     * @return Reputation data
     */
    FactionReputation getReputation(const std::string& factionId) const;
    
    /**
     * Set reputation value with a faction
     * @param factionId Faction identifier
     * @param value Reputation value (-1000 to +1000)
     */
    void setReputation(const std::string& factionId, int value);
    
    /**
     * Change reputation with a faction
     * @param factionId Faction identifier
     * @param delta Change amount
     * @param reason Reason for change (for logging/events)
     */
    void changeReputation(const std::string& factionId, int delta, const std::string& reason = "");
    
    /**
     * Get reputation level with a faction
     * @param factionId Faction identifier
     * @return Reputation level
     */
    ReputationLevel getReputationLevel(const std::string& factionId) const;
    
    /**
     * Get all factions player has reputation with
     * @return Map of faction IDs to reputation data
     */
    std::unordered_map<std::string, FactionReputation> getAllReputations() const;
    
    // Faction relationships
    
    /**
     * Set relationship between two factions
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     * @param relationship Relationship type
     */
    void setFactionRelationship(const std::string& faction1Id,
                                const std::string& faction2Id,
                                FactionRelationship relationship);
    
    /**
     * Get relationship between two factions
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     * @return Relationship type
     */
    FactionRelationship getFactionRelationship(const std::string& faction1Id,
                                               const std::string& faction2Id) const;
    
    /**
     * Check if two factions are allies
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     * @return True if allied
     */
    bool areAllies(const std::string& faction1Id, const std::string& faction2Id) const;
    
    /**
     * Check if two factions are enemies
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     * @return True if enemies
     */
    bool areEnemies(const std::string& faction1Id, const std::string& faction2Id) const;
    
    /**
     * Get all allies of a faction
     * @param factionId Faction identifier
     * @return Vector of allied faction IDs
     */
    std::vector<std::string> getAllies(const std::string& factionId) const;
    
    /**
     * Get all enemies of a faction
     * @param factionId Faction identifier
     * @return Vector of enemy faction IDs
     */
    std::vector<std::string> getEnemies(const std::string& factionId) const;
    
    // War and conflict
    
    /**
     * Declare war between two factions
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     */
    void declareWar(const std::string& faction1Id, const std::string& faction2Id);
    
    /**
     * Make peace between two factions
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     */
    void makePeace(const std::string& faction1Id, const std::string& faction2Id);
    
    /**
     * Check if two factions are at war
     * @param faction1Id First faction ID
     * @param faction2Id Second faction ID
     * @return True if at war
     */
    bool atWar(const std::string& faction1Id, const std::string& faction2Id) const;
    
    // Membership
    
    /**
     * Join a faction
     * @param factionId Faction to join
     * @param rank Initial rank (0 = lowest)
     * @return True if joined successfully
     */
    bool joinFaction(const std::string& factionId, int rank = 0);
    
    /**
     * Leave a faction
     * @param factionId Faction to leave
     */
    void leaveFaction(const std::string& factionId);
    
    /**
     * Check if player is member of a faction
     * @param factionId Faction identifier
     * @return True if member
     */
    bool isMember(const std::string& factionId) const;
    
    /**
     * Get player's rank in a faction
     * @param factionId Faction identifier
     * @return Rank (0 if not member)
     */
    int getMemberRank(const std::string& factionId) const;
    
    /**
     * Promote player in a faction
     * @param factionId Faction identifier
     * @return New rank, or -1 if failed
     */
    int promote(const std::string& factionId);
    
    /**
     * Demote player in a faction
     * @param factionId Faction identifier
     * @return New rank, or -1 if failed
     */
    int demote(const std::string& factionId);
    
    // Entity faction assignment
    
    /**
     * Assign an entity to a faction
     * @param entity Entity to assign
     * @param factionId Faction identifier
     */
    void assignEntityToFaction(Entity* entity, const std::string& factionId);
    
    /**
     * Get entity's faction
     * @param entity Entity to query
     * @return Faction ID, empty if no faction
     */
    std::string getEntityFaction(Entity* entity) const;
    
    /**
     * Get all entities in a faction
     * @param factionId Faction identifier
     * @return Vector of entities
     */
    std::vector<Entity*> getEntitiesInFaction(const std::string& factionId) const;
    
    // Callbacks
    
    /**
     * Set callback for reputation changes
     * @param callback Function to call when reputation changes
     */
    void setReputationCallback(std::function<void(const ReputationEvent&)> callback);
    
    /**
     * Set callback for faction relationship changes
     * @param callback Function to call when relationships change
     */
    void setRelationshipCallback(std::function<void(const std::string&, const std::string&)> callback);
    
    // Save/Load
    
    /**
     * Save faction data and reputation
     * @param filepath Path to save file
     * @return True if saved successfully
     */
    bool save(const std::string& filepath);
    
    /**
     * Load faction data and reputation
     * @param filepath Path to load file
     * @return True if loaded successfully
     */
    bool load(const std::string& filepath);
    
    // Statistics
    
    /**
     * Statistics about faction system
     */
    struct Statistics {
        int totalFactions = 0;
        int discoveredFactions = 0;
        int joinedFactions = 0;
        int alliedFactions = 0;
        int hostileFactions = 0;
        int activeWars = 0;
    };
    
    /**
     * Get faction statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;

private:
    // Faction definitions
    std::unordered_map<std::string, Faction> m_factions;
    
    // Player reputation
    std::unordered_map<std::string, FactionReputation> m_reputations;
    
    // Faction relationships
    std::unordered_map<std::string, FactionRelationshipData> m_relationships;
    
    // Entity assignments
    std::unordered_map<Entity*, std::string> m_entityFactions;
    
    // Callbacks
    std::function<void(const ReputationEvent&)> m_reputationCallback;
    std::function<void(const std::string&, const std::string&)> m_relationshipCallback;
    
    // Internal methods
    ReputationLevel calculateReputationLevel(int value) const;
    std::string getRelationshipKey(const std::string& faction1, const std::string& faction2) const;
    void notifyReputationChange(const ReputationEvent& event);
    void propagateReputationChange(const std::string& factionId, int delta);
};

// Helper functions
namespace FactionHelpers {
    /**
     * Get string name for reputation level
     * @param level Reputation level
     * @return String name
     */
    const char* getReputationLevelName(ReputationLevel level);
    
    /**
     * Get string name for faction relationship
     * @param relationship Relationship type
     * @return String name
     */
    const char* getFactionRelationshipName(FactionRelationship relationship);
    
    /**
     * Get reputation value required for level
     * @param level Reputation level
     * @return Minimum reputation value for that level
     */
    int getReputationThreshold(ReputationLevel level);
}

} // namespace Gameplay
} // namespace JJM
