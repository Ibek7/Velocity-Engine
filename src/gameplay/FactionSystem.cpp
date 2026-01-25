#include "gameplay/FactionSystem.h"
#include <algorithm>
#include <fstream>

namespace JJM {
namespace Gameplay {

FactionSystem::FactionSystem() {
}

FactionSystem::~FactionSystem() {
    shutdown();
}

void FactionSystem::initialize() {
}

void FactionSystem::shutdown() {
    m_factions.clear();
    m_reputations.clear();
    m_relationships.clear();
    m_entityFactions.clear();
}

void FactionSystem::update(float deltaTime) {
    // Could implement reputation decay, dynamic relationship changes, etc.
}

bool FactionSystem::registerFaction(const Faction& faction) {
    if (faction.id.empty()) return false;
    
    m_factions[faction.id] = faction;
    
    // Initialize reputation
    FactionReputation rep;
    rep.factionId = faction.id;
    rep.reputationValue = 0;
    rep.level = ReputationLevel::NEUTRAL;
    m_reputations[faction.id] = rep;
    
    return true;
}

int FactionSystem::loadFactions(const std::string& filepath) {
    // TODO: Implement JSON/XML loading
    return 0;
}

Faction* FactionSystem::getFaction(const std::string& id) {
    auto it = m_factions.find(id);
    return (it != m_factions.end()) ? &it->second : nullptr;
}

std::vector<Faction*> FactionSystem::getAllFactions() {
    std::vector<Faction*> result;
    for (auto& pair : m_factions) {
        result.push_back(&pair.second);
    }
    return result;
}

std::vector<Faction*> FactionSystem::getDiscoveredFactions() {
    std::vector<Faction*> result;
    for (auto& pair : m_factions) {
        if (!pair.second.hidden) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

void FactionSystem::discoverFaction(const std::string& id) {
    auto* faction = getFaction(id);
    if (faction) {
        faction->hidden = false;
    }
}

FactionReputation FactionSystem::getReputation(const std::string& factionId) const {
    auto it = m_reputations.find(factionId);
    if (it != m_reputations.end()) {
        return it->second;
    }
    
    FactionReputation rep;
    rep.factionId = factionId;
    return rep;
}

void FactionSystem::setReputation(const std::string& factionId, int value) {
    value = std::max(-1000, std::min(1000, value));
    
    auto& rep = m_reputations[factionId];
    int oldValue = rep.reputationValue;
    ReputationLevel oldLevel = rep.level;
    
    rep.factionId = factionId;
    rep.reputationValue = value;
    rep.level = calculateReputationLevel(value);
    
    if (oldValue != value || oldLevel != rep.level) {
        ReputationEvent event;
        event.factionId = factionId;
        event.oldValue = oldValue;
        event.newValue = value;
        event.oldLevel = oldLevel;
        event.newLevel = rep.level;
        
        notifyReputationChange(event);
    }
}

void FactionSystem::changeReputation(const std::string& factionId, int delta,
                                     const std::string& reason) {
    auto& rep = m_reputations[factionId];
    int newValue = rep.reputationValue + delta;
    setReputation(factionId, newValue);
    
    // Propagate to allied/enemy factions
    propagateReputationChange(factionId, delta);
}

ReputationLevel FactionSystem::getReputationLevel(const std::string& factionId) const {
    return getReputation(factionId).level;
}

std::unordered_map<std::string, FactionReputation> FactionSystem::getAllReputations() const {
    return m_reputations;
}

void FactionSystem::setFactionRelationship(const std::string& faction1Id,
                                           const std::string& faction2Id,
                                           FactionRelationship relationship) {
    std::string key = getRelationshipKey(faction1Id, faction2Id);
    
    FactionRelationshipData data;
    data.faction1Id = faction1Id;
    data.faction2Id = faction2Id;
    data.relationship = relationship;
    
    switch (relationship) {
        case FactionRelationship::ALLIED:
            data.relationshipValue = 100;
            break;
        case FactionRelationship::FRIENDLY:
            data.relationshipValue = 50;
            break;
        case FactionRelationship::NEUTRAL:
            data.relationshipValue = 0;
            break;
        case FactionRelationship::UNFRIENDLY:
            data.relationshipValue = -50;
            break;
        case FactionRelationship::HOSTILE:
            data.relationshipValue = -75;
            break;
        case FactionRelationship::ENEMY:
            data.relationshipValue = -100;
            data.atWar = true;
            break;
    }
    
    m_relationships[key] = data;
    
    if (m_relationshipCallback) {
        m_relationshipCallback(faction1Id, faction2Id);
    }
}

FactionRelationship FactionSystem::getFactionRelationship(const std::string& faction1Id,
                                                          const std::string& faction2Id) const {
    std::string key = getRelationshipKey(faction1Id, faction2Id);
    auto it = m_relationships.find(key);
    
    if (it != m_relationships.end()) {
        return it->second.relationship;
    }
    
    return FactionRelationship::NEUTRAL;
}

bool FactionSystem::areAllies(const std::string& faction1Id, const std::string& faction2Id) const {
    FactionRelationship rel = getFactionRelationship(faction1Id, faction2Id);
    return rel == FactionRelationship::ALLIED;
}

bool FactionSystem::areEnemies(const std::string& faction1Id, const std::string& faction2Id) const {
    FactionRelationship rel = getFactionRelationship(faction1Id, faction2Id);
    return rel == FactionRelationship::HOSTILE || rel == FactionRelationship::ENEMY;
}

std::vector<std::string> FactionSystem::getAllies(const std::string& factionId) const {
    std::vector<std::string> allies;
    for (const auto& pair : m_relationships) {
        const auto& data = pair.second;
        if (data.relationship == FactionRelationship::ALLIED) {
            if (data.faction1Id == factionId) {
                allies.push_back(data.faction2Id);
            } else if (data.faction2Id == factionId) {
                allies.push_back(data.faction1Id);
            }
        }
    }
    return allies;
}

std::vector<std::string> FactionSystem::getEnemies(const std::string& factionId) const {
    std::vector<std::string> enemies;
    for (const auto& pair : m_relationships) {
        const auto& data = pair.second;
        if (data.relationship == FactionRelationship::HOSTILE ||
            data.relationship == FactionRelationship::ENEMY) {
            if (data.faction1Id == factionId) {
                enemies.push_back(data.faction2Id);
            } else if (data.faction2Id == factionId) {
                enemies.push_back(data.faction1Id);
            }
        }
    }
    return enemies;
}

void FactionSystem::declareWar(const std::string& faction1Id, const std::string& faction2Id) {
    setFactionRelationship(faction1Id, faction2Id, FactionRelationship::ENEMY);
}

void FactionSystem::makePeace(const std::string& faction1Id, const std::string& faction2Id) {
    setFactionRelationship(faction1Id, faction2Id, FactionRelationship::NEUTRAL);
}

bool FactionSystem::atWar(const std::string& faction1Id, const std::string& faction2Id) const {
    std::string key = getRelationshipKey(faction1Id, faction2Id);
    auto it = m_relationships.find(key);
    return (it != m_relationships.end()) && it->second.atWar;
}

bool FactionSystem::joinFaction(const std::string& factionId, int rank) {
    auto* faction = getFaction(factionId);
    if (!faction || !faction->canJoin) return false;
    
    auto& rep = m_reputations[factionId];
    if (rep.isMember) return false;  // Already member
    
    rep.isMember = true;
    rep.memberRank = rank;
    
    return true;
}

void FactionSystem::leaveFaction(const std::string& factionId) {
    auto& rep = m_reputations[factionId];
    rep.isMember = false;
    rep.memberRank = 0;
}

bool FactionSystem::isMember(const std::string& factionId) const {
    return getReputation(factionId).isMember;
}

int FactionSystem::getMemberRank(const std::string& factionId) const {
    return getReputation(factionId).memberRank;
}

int FactionSystem::promote(const std::string& factionId) {
    auto& rep = m_reputations[factionId];
    if (!rep.isMember) return -1;
    
    rep.memberRank++;
    return rep.memberRank;
}

int FactionSystem::demote(const std::string& factionId) {
    auto& rep = m_reputations[factionId];
    if (!rep.isMember || rep.memberRank <= 0) return -1;
    
    rep.memberRank--;
    return rep.memberRank;
}

void FactionSystem::assignEntityToFaction(Entity* entity, const std::string& factionId) {
    m_entityFactions[entity] = factionId;
}

std::string FactionSystem::getEntityFaction(Entity* entity) const {
    auto it = m_entityFactions.find(entity);
    return (it != m_entityFactions.end()) ? it->second : "";
}

std::vector<Entity*> FactionSystem::getEntitiesInFaction(const std::string& factionId) const {
    std::vector<Entity*> result;
    for (const auto& pair : m_entityFactions) {
        if (pair.second == factionId) {
            result.push_back(pair.first);
        }
    }
    return result;
}

void FactionSystem::setReputationCallback(std::function<void(const ReputationEvent&)> callback) {
    m_reputationCallback = callback;
}

void FactionSystem::setRelationshipCallback(std::function<void(const std::string&, const std::string&)> callback) {
    m_relationshipCallback = callback;
}

bool FactionSystem::save(const std::string& filepath) {
    // TODO: Implement serialization
    return false;
}

bool FactionSystem::load(const std::string& filepath) {
    // TODO: Implement deserialization
    return false;
}

FactionSystem::Statistics FactionSystem::getStatistics() const {
    Statistics stats;
    stats.totalFactions = static_cast<int>(m_factions.size());
    
    for (const auto& pair : m_factions) {
        if (!pair.second.hidden) {
            stats.discoveredFactions++;
        }
    }
    
    for (const auto& pair : m_reputations) {
        if (pair.second.isMember) {
            stats.joinedFactions++;
        }
        
        if (pair.second.level == ReputationLevel::EXALTED ||
            pair.second.level == ReputationLevel::REVERED) {
            stats.alliedFactions++;
        }
        
        if (pair.second.level == ReputationLevel::HOSTILE ||
            pair.second.level == ReputationLevel::HATED) {
            stats.hostileFactions++;
        }
    }
    
    for (const auto& pair : m_relationships) {
        if (pair.second.atWar) {
            stats.activeWars++;
        }
    }
    
    return stats;
}

ReputationLevel FactionSystem::calculateReputationLevel(int value) const {
    if (value >= 900) return ReputationLevel::REVERED;
    if (value >= 600) return ReputationLevel::EXALTED;
    if (value >= 300) return ReputationLevel::HONORED;
    if (value >= 100) return ReputationLevel::FRIENDLY;
    if (value >= -100) return ReputationLevel::NEUTRAL;
    if (value >= -300) return ReputationLevel::UNFRIENDLY;
    if (value >= -600) return ReputationLevel::HOSTILE;
    return ReputationLevel::HATED;
}

std::string FactionSystem::getRelationshipKey(const std::string& faction1,
                                              const std::string& faction2) const {
    // Ensure consistent ordering
    if (faction1 < faction2) {
        return faction1 + "_" + faction2;
    } else {
        return faction2 + "_" + faction1;
    }
}

void FactionSystem::notifyReputationChange(const ReputationEvent& event) {
    if (m_reputationCallback) {
        m_reputationCallback(event);
    }
}

void FactionSystem::propagateReputationChange(const std::string& factionId, int delta) {
    // Gain/lose reputation with allies and enemies
    auto allies = getAllies(factionId);
    auto enemies = getEnemies(factionId);
    
    // Gain reputation with allies (50% of original)
    for (const auto& allyId : allies) {
        auto& rep = m_reputations[allyId];
        int allyDelta = delta / 2;
        rep.reputationValue = std::max(-1000, std::min(1000, rep.reputationValue + allyDelta));
        rep.level = calculateReputationLevel(rep.reputationValue);
    }
    
    // Lose reputation with enemies (opposite effect, 25%)
    for (const auto& enemyId : enemies) {
        auto& rep = m_reputations[enemyId];
        int enemyDelta = -delta / 4;
        rep.reputationValue = std::max(-1000, std::min(1000, rep.reputationValue + enemyDelta));
        rep.level = calculateReputationLevel(rep.reputationValue);
    }
}

// Helper functions
namespace FactionHelpers {
    const char* getReputationLevelName(ReputationLevel level) {
        switch (level) {
            case ReputationLevel::REVERED: return "Revered";
            case ReputationLevel::EXALTED: return "Exalted";
            case ReputationLevel::HONORED: return "Honored";
            case ReputationLevel::FRIENDLY: return "Friendly";
            case ReputationLevel::NEUTRAL: return "Neutral";
            case ReputationLevel::UNFRIENDLY: return "Unfriendly";
            case ReputationLevel::HOSTILE: return "Hostile";
            case ReputationLevel::HATED: return "Hated";
            default: return "Unknown";
        }
    }
    
    const char* getFactionRelationshipName(FactionRelationship relationship) {
        switch (relationship) {
            case FactionRelationship::ALLIED: return "Allied";
            case FactionRelationship::FRIENDLY: return "Friendly";
            case FactionRelationship::NEUTRAL: return "Neutral";
            case FactionRelationship::UNFRIENDLY: return "Unfriendly";
            case FactionRelationship::HOSTILE: return "Hostile";
            case FactionRelationship::ENEMY: return "Enemy";
            default: return "Unknown";
        }
    }
    
    int getReputationThreshold(ReputationLevel level) {
        switch (level) {
            case ReputationLevel::REVERED: return 900;
            case ReputationLevel::EXALTED: return 600;
            case ReputationLevel::HONORED: return 300;
            case ReputationLevel::FRIENDLY: return 100;
            case ReputationLevel::NEUTRAL: return -100;
            case ReputationLevel::UNFRIENDLY: return -300;
            case ReputationLevel::HOSTILE: return -600;
            case ReputationLevel::HATED: return -1000;
            default: return 0;
        }
    }
}

} // namespace Gameplay
} // namespace JJM
