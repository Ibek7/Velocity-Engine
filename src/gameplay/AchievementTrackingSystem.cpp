#include "gameplay/AchievementTrackingSystem.h"
#include <algorithm>
#include <ctime>

namespace JJM {
namespace Gameplay {

// Constructor
AchievementTrackingSystem::AchievementTrackingSystem()
    : m_enabled(true)
    , m_notificationsEnabled(true)
    , m_offlineTracking(false)
{
}

// Destructor
AchievementTrackingSystem::~AchievementTrackingSystem() {
    shutdown();
}

// Initialize
void AchievementTrackingSystem::initialize() {
    m_achievementDefinitions.clear();
    m_playerAchievements.clear();
}

// Shutdown
void AchievementTrackingSystem::shutdown() {
    m_achievementDefinitions.clear();
    m_playerAchievements.clear();
    m_unlockCallback = nullptr;
    m_progressCallback = nullptr;
}

// Update
void AchievementTrackingSystem::update(float deltaTime) {
    if (!m_enabled) return;
    
    // Update time-based achievements
    // (Could track time-limited challenges here)
}

// Register achievement
void AchievementTrackingSystem::registerAchievement(const Achievement& achievement) {
    m_achievementDefinitions[achievement.id] = achievement;
}

// Load achievements
int AchievementTrackingSystem::loadAchievements(const std::string& filepath) {
    // TODO: Implement file loading
    return 0;
}

// Get achievement
const Achievement* AchievementTrackingSystem::getAchievement(const std::string& achievementId) const {
    auto it = m_achievementDefinitions.find(achievementId);
    if (it != m_achievementDefinitions.end()) {
        return &it->second;
    }
    return nullptr;
}

// Get all achievements
std::vector<Achievement> AchievementTrackingSystem::getAllAchievements() const {
    std::vector<Achievement> achievements;
    achievements.reserve(m_achievementDefinitions.size());
    
    for (const auto& pair : m_achievementDefinitions) {
        achievements.push_back(pair.second);
    }
    
    return achievements;
}

// Get achievements by category
std::vector<Achievement> AchievementTrackingSystem::getAchievementsByCategory(AchievementCategory category) const {
    std::vector<Achievement> achievements;
    
    for (const auto& pair : m_achievementDefinitions) {
        if (pair.second.category == category) {
            achievements.push_back(pair.second);
        }
    }
    
    return achievements;
}

// Initialize player
void AchievementTrackingSystem::initializePlayer(int playerId) {
    if (m_playerAchievements.find(playerId) != m_playerAchievements.end()) {
        return; // Already initialized
    }
    
    // Copy all achievement definitions
    m_playerAchievements[playerId] = m_achievementDefinitions;
}

// Get player achievement
Achievement* AchievementTrackingSystem::getPlayerAchievement(int playerId, const std::string& achievementId) {
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return nullptr;
    }
    
    auto achIt = playerIt->second.find(achievementId);
    if (achIt != playerIt->second.end()) {
        return &achIt->second;
    }
    
    return nullptr;
}

// Get player achievements
std::vector<Achievement> AchievementTrackingSystem::getPlayerAchievements(int playerId) const {
    std::vector<Achievement> achievements;
    
    auto it = m_playerAchievements.find(playerId);
    if (it != m_playerAchievements.end()) {
        achievements.reserve(it->second.size());
        for (const auto& pair : it->second) {
            achievements.push_back(pair.second);
        }
    }
    
    return achievements;
}

// Get unlocked achievements
std::vector<Achievement> AchievementTrackingSystem::getUnlockedAchievements(int playerId) const {
    std::vector<Achievement> achievements;
    
    auto it = m_playerAchievements.find(playerId);
    if (it != m_playerAchievements.end()) {
        for (const auto& pair : it->second) {
            if (pair.second.state == AchievementState::UNLOCKED) {
                achievements.push_back(pair.second);
            }
        }
    }
    
    return achievements;
}

// Get locked achievements
std::vector<Achievement> AchievementTrackingSystem::getLockedAchievements(int playerId) const {
    std::vector<Achievement> achievements;
    
    auto it = m_playerAchievements.find(playerId);
    if (it != m_playerAchievements.end()) {
        for (const auto& pair : it->second) {
            if (pair.second.state != AchievementState::UNLOCKED) {
                achievements.push_back(pair.second);
            }
        }
    }
    
    return achievements;
}

// Track event
void AchievementTrackingSystem::trackEvent(int playerId, const std::string& eventId, int value) {
    if (!m_enabled) return;
    
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return;
    }
    
    // Check all achievements for this event
    for (auto& achPair : playerIt->second) {
        Achievement& achievement = achPair.second;
        
        // Skip if already unlocked
        if (achievement.state == AchievementState::UNLOCKED) {
            continue;
        }
        
        // Check conditions
        for (auto& condition : achievement.conditions) {
            if (condition.eventId == eventId && !condition.completed) {
                updateCondition(playerId, achievement, condition, value);
            }
        }
    }
}

// Unlock achievement
bool AchievementTrackingSystem::unlockAchievement(int playerId, const std::string& achievementId) {
    Achievement* achievement = getPlayerAchievement(playerId, achievementId);
    if (!achievement) return false;
    
    if (achievement->state == AchievementState::UNLOCKED) {
        return false; // Already unlocked
    }
    
    // Mark as unlocked
    achievement->state = AchievementState::UNLOCKED;
    achievement->progress = 1.0f;
    achievement->unlockedTimestamp = static_cast<int>(std::time(nullptr));
    
    // Mark all conditions as completed
    for (auto& condition : achievement->conditions) {
        condition.completed = true;
        condition.currentValue = condition.targetValue;
    }
    
    // Grant reward
    grantAchievementReward(playerId, achievement->reward);
    
    // Callback
    if (m_unlockCallback && m_notificationsEnabled) {
        AchievementUnlockEvent event;
        event.achievementId = achievement->id;
        event.achievementName = achievement->name;
        event.points = achievement->points;
        event.reward = achievement->reward;
        event.playerId = playerId;
        m_unlockCallback(event);
    }
    
    return true;
}

// Check if unlocked
bool AchievementTrackingSystem::isAchievementUnlocked(int playerId, const std::string& achievementId) const {
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return false;
    }
    
    auto achIt = playerIt->second.find(achievementId);
    if (achIt != playerIt->second.end()) {
        return achIt->second.state == AchievementState::UNLOCKED;
    }
    
    return false;
}

// Get progress
float AchievementTrackingSystem::getAchievementProgress(int playerId, const std::string& achievementId) const {
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return 0.0f;
    }
    
    auto achIt = playerIt->second.find(achievementId);
    if (achIt != playerIt->second.end()) {
        return achIt->second.progress;
    }
    
    return 0.0f;
}

// Reset achievement
void AchievementTrackingSystem::resetAchievement(int playerId, const std::string& achievementId) {
    Achievement* achievement = getPlayerAchievement(playerId, achievementId);
    if (!achievement) return;
    
    // Reset to definition defaults
    const Achievement* definition = getAchievement(achievementId);
    if (definition) {
        *achievement = *definition;
    }
}

// Reset all achievements
void AchievementTrackingSystem::resetAllAchievements(int playerId) {
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt != m_playerAchievements.end()) {
        playerIt->second = m_achievementDefinitions;
    }
}

// Get player stats
PlayerAchievementStats AchievementTrackingSystem::getPlayerStats(int playerId) const {
    PlayerAchievementStats stats;
    
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return stats;
    }
    
    stats.totalAchievements = playerIt->second.size();
    
    for (const auto& pair : playerIt->second) {
        const Achievement& ach = pair.second;
        
        stats.totalPoints += ach.points;
        
        if (ach.state == AchievementState::UNLOCKED) {
            stats.unlockedAchievements++;
            stats.earnedPoints += ach.points;
        }
        
        // Category progress
        stats.categoryProgress[ach.category]++;
    }
    
    if (stats.totalAchievements > 0) {
        stats.completionPercentage = (float)stats.unlockedAchievements / stats.totalAchievements * 100.0f;
    }
    
    return stats;
}

// Get total points
int AchievementTrackingSystem::getTotalPoints(int playerId) const {
    int points = 0;
    
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt != m_playerAchievements.end()) {
        for (const auto& pair : playerIt->second) {
            if (pair.second.state == AchievementState::UNLOCKED) {
                points += pair.second.points;
            }
        }
    }
    
    return points;
}

// Get completion percentage
float AchievementTrackingSystem::getCompletionPercentage(int playerId) const {
    auto playerIt = m_playerAchievements.find(playerId);
    if (playerIt == m_playerAchievements.end()) {
        return 0.0f;
    }
    
    int total = playerIt->second.size();
    if (total == 0) return 0.0f;
    
    int unlocked = 0;
    for (const auto& pair : playerIt->second) {
        if (pair.second.state == AchievementState::UNLOCKED) {
            unlocked++;
        }
    }
    
    return (float)unlocked / total * 100.0f;
}

// Get recently unlocked
std::vector<Achievement> AchievementTrackingSystem::getRecentlyUnlocked(int playerId, int count) const {
    std::vector<Achievement> achievements = getUnlockedAchievements(playerId);
    
    // Sort by unlock timestamp (descending)
    std::sort(achievements.begin(), achievements.end(), 
        [](const Achievement& a, const Achievement& b) {
            return a.unlockedTimestamp > b.unlockedTimestamp;
        });
    
    // Return first N
    if (achievements.size() > static_cast<size_t>(count)) {
        achievements.resize(count);
    }
    
    return achievements;
}

// Save player achievements
bool AchievementTrackingSystem::savePlayerAchievements(int playerId, const std::string& filepath) {
    // TODO: Implement save
    return false;
}

// Load player achievements
bool AchievementTrackingSystem::loadPlayerAchievements(int playerId, const std::string& filepath) {
    // TODO: Implement load
    return false;
}

// Callbacks

void AchievementTrackingSystem::setAchievementUnlockedCallback(std::function<void(const AchievementUnlockEvent&)> callback) {
    m_unlockCallback = callback;
}

void AchievementTrackingSystem::setAchievementProgressCallback(std::function<void(const AchievementProgressEvent&)> callback) {
    m_progressCallback = callback;
}

// Utility methods

const char* AchievementTrackingSystem::getCategoryName(AchievementCategory category) {
    switch (category) {
        case AchievementCategory::STORY: return "Story";
        case AchievementCategory::COMBAT: return "Combat";
        case AchievementCategory::EXPLORATION: return "Exploration";
        case AchievementCategory::COLLECTION: return "Collection";
        case AchievementCategory::SKILL: return "Skill";
        case AchievementCategory::SOCIAL: return "Social";
        case AchievementCategory::SPECIAL: return "Special";
        case AchievementCategory::MISCELLANEOUS: return "Miscellaneous";
        default: return "Unknown";
    }
}

const char* AchievementTrackingSystem::getConditionTypeName(ConditionType type) {
    switch (type) {
        case ConditionType::SINGLE_EVENT: return "Single Event";
        case ConditionType::CUMULATIVE: return "Cumulative";
        case ConditionType::THRESHOLD: return "Threshold";
        case ConditionType::COLLECTION: return "Collection";
        case ConditionType::COMBO: return "Combo";
        case ConditionType::TIME_LIMITED: return "Time Limited";
        default: return "Unknown";
    }
}

// Internal methods

void AchievementTrackingSystem::updateCondition(int playerId, Achievement& achievement, AchievementCondition& condition, int value) {
    switch (condition.type) {
        case ConditionType::SINGLE_EVENT:
            condition.completed = true;
            condition.currentValue = 1;
            break;
            
        case ConditionType::CUMULATIVE:
            condition.currentValue += value;
            if (condition.currentValue >= condition.targetValue) {
                condition.completed = true;
                condition.currentValue = condition.targetValue;
            }
            break;
            
        case ConditionType::THRESHOLD:
            condition.currentValue = value;
            if (condition.currentValue >= condition.targetValue) {
                condition.completed = true;
            }
            break;
            
        default:
            break;
    }
    
    // Update achievement progress
    achievement.progress = calculateProgress(achievement);
    
    // Progress callback
    if (m_progressCallback && m_notificationsEnabled) {
        AchievementProgressEvent event;
        event.achievementId = achievement.id;
        event.conditionId = condition.id;
        event.currentValue = condition.currentValue;
        event.targetValue = condition.targetValue;
        event.progress = achievement.progress;
        event.playerId = playerId;
        m_progressCallback(event);
    }
    
    // Check if achievement is complete
    checkAchievementCompletion(playerId, achievement);
}

void AchievementTrackingSystem::checkAchievementCompletion(int playerId, Achievement& achievement) {
    // Check if all conditions are completed
    bool allCompleted = true;
    for (const auto& condition : achievement.conditions) {
        if (!condition.completed) {
            allCompleted = false;
            break;
        }
    }
    
    if (allCompleted) {
        unlockAchievement(playerId, achievement.id);
    }
}

void AchievementTrackingSystem::grantAchievementReward(int playerId, const AchievementReward& reward) {
    // TODO: Integrate with game systems to grant rewards
    // - Grant experience
    // - Grant currency
    // - Grant items
    // - Unlock title
}

float AchievementTrackingSystem::calculateProgress(const Achievement& achievement) const {
    if (achievement.conditions.empty()) return 0.0f;
    
    float totalProgress = 0.0f;
    
    for (const auto& condition : achievement.conditions) {
        if (condition.targetValue > 0) {
            float conditionProgress = std::min(1.0f, (float)condition.currentValue / condition.targetValue);
            totalProgress += conditionProgress;
        }
    }
    
    return totalProgress / achievement.conditions.size();
}

} // namespace Gameplay
} // namespace JJM
