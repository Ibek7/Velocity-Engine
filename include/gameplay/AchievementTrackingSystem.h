#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Gameplay {

/**
 * Achievement categories
 */
enum class AchievementCategory {
    STORY,              // Story progression
    COMBAT,             // Combat-related
    EXPLORATION,        // Discovery/exploration
    COLLECTION,         // Collecting items/objects
    SKILL,              // Skill-based challenges
    SOCIAL,             // Multiplayer/social
    SPECIAL,            // Special/hidden achievements
    MISCELLANEOUS       // Other
};

/**
 * Achievement state
 */
enum class AchievementState {
    LOCKED,             // Not yet unlocked
    IN_PROGRESS,        // Progress tracked but not complete
    UNLOCKED,           // Completed
    HIDDEN              // Hidden until revealed
};

/**
 * Achievement condition types
 */
enum class ConditionType {
    SINGLE_EVENT,       // Single event trigger
    CUMULATIVE,         // Accumulate count over time
    THRESHOLD,          // Reach specific value
    COLLECTION,         // Collect all items in set
    COMBO,              // Multiple conditions required
    TIME_LIMITED        // Must complete within time limit
};

/**
 * Achievement reward
 */
struct AchievementReward {
    int experience = 0;
    int currency = 0;
    std::vector<std::string> items;     // Item IDs to grant
    std::string title;                  // Unlocked title
    std::string customData;             // Game-specific data
};

/**
 * Achievement progress condition
 */
struct AchievementCondition {
    std::string id;
    ConditionType type = ConditionType::SINGLE_EVENT;
    std::string eventId;                // Event to track
    int currentValue = 0;
    int targetValue = 1;
    bool completed = false;
};

/**
 * Complete achievement definition
 */
struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    std::string iconPath;
    
    AchievementCategory category = AchievementCategory::MISCELLANEOUS;
    AchievementState state = AchievementState::LOCKED;
    
    int points = 10;                    // Achievement points
    bool isSecret = false;              // Hidden until unlocked
    bool isRare = false;                // Marked as rare/hard
    
    std::vector<AchievementCondition> conditions;
    AchievementReward reward;
    
    // Progress tracking
    float progress = 0.0f;              // 0-1 (overall progress)
    int unlockedTimestamp = 0;          // Unix timestamp when unlocked
};

/**
 * Achievement unlock event
 */
struct AchievementUnlockEvent {
    std::string achievementId;
    std::string achievementName;
    int points;
    AchievementReward reward;
    int playerId;
};

/**
 * Achievement progress event
 */
struct AchievementProgressEvent {
    std::string achievementId;
    std::string conditionId;
    int currentValue;
    int targetValue;
    float progress;
    int playerId;
};

/**
 * Player achievement statistics
 */
struct PlayerAchievementStats {
    int totalAchievements = 0;
    int unlockedAchievements = 0;
    int totalPoints = 0;
    int earnedPoints = 0;
    
    float completionPercentage = 0.0f;
    
    std::unordered_map<AchievementCategory, int> categoryProgress;
};

/**
 * System for tracking and managing achievements
 */
class AchievementTrackingSystem {
public:
    AchievementTrackingSystem();
    ~AchievementTrackingSystem();
    
    /**
     * Initialize the system
     */
    void initialize();
    
    /**
     * Shutdown the system
     */
    void shutdown();
    
    /**
     * Update achievement tracking
     * @param deltaTime Time since last update
     */
    void update(float deltaTime);
    
    // Achievement definitions
    
    /**
     * Register achievement definition
     * @param achievement Achievement to register
     */
    void registerAchievement(const Achievement& achievement);
    
    /**
     * Load achievements from file
     * @param filepath Path to achievements file
     * @return Number of achievements loaded
     */
    int loadAchievements(const std::string& filepath);
    
    /**
     * Get achievement definition
     * @param achievementId Achievement ID
     * @return Pointer to achievement, or nullptr
     */
    const Achievement* getAchievement(const std::string& achievementId) const;
    
    /**
     * Get all achievements
     * @return Vector of all achievements
     */
    std::vector<Achievement> getAllAchievements() const;
    
    /**
     * Get achievements by category
     * @param category Category to filter by
     * @return Vector of achievements in category
     */
    std::vector<Achievement> getAchievementsByCategory(AchievementCategory category) const;
    
    // Player progress
    
    /**
     * Initialize player achievements
     * @param playerId Player ID
     */
    void initializePlayer(int playerId);
    
    /**
     * Get player achievement data
     * @param playerId Player ID
     * @param achievementId Achievement ID
     * @return Pointer to player's achievement data, or nullptr
     */
    Achievement* getPlayerAchievement(int playerId, const std::string& achievementId);
    
    /**
     * Get all player achievements
     * @param playerId Player ID
     * @return Vector of player's achievements
     */
    std::vector<Achievement> getPlayerAchievements(int playerId) const;
    
    /**
     * Get player unlocked achievements
     * @param playerId Player ID
     * @return Vector of unlocked achievements
     */
    std::vector<Achievement> getUnlockedAchievements(int playerId) const;
    
    /**
     * Get player locked achievements
     * @param playerId Player ID
     * @return Vector of locked achievements
     */
    std::vector<Achievement> getLockedAchievements(int playerId) const;
    
    // Progress tracking
    
    /**
     * Track event for achievement progress
     * @param playerId Player ID
     * @param eventId Event ID
     * @param value Value to add/set (depends on condition type)
     */
    void trackEvent(int playerId, const std::string& eventId, int value = 1);
    
    /**
     * Manually unlock achievement
     * @param playerId Player ID
     * @param achievementId Achievement ID
     * @return True if unlocked
     */
    bool unlockAchievement(int playerId, const std::string& achievementId);
    
    /**
     * Check if achievement is unlocked
     * @param playerId Player ID
     * @param achievementId Achievement ID
     * @return True if unlocked
     */
    bool isAchievementUnlocked(int playerId, const std::string& achievementId) const;
    
    /**
     * Get achievement progress
     * @param playerId Player ID
     * @param achievementId Achievement ID
     * @return Progress (0-1)
     */
    float getAchievementProgress(int playerId, const std::string& achievementId) const;
    
    /**
     * Reset player progress for achievement
     * @param playerId Player ID
     * @param achievementId Achievement ID
     */
    void resetAchievement(int playerId, const std::string& achievementId);
    
    /**
     * Reset all player achievements
     * @param playerId Player ID
     */
    void resetAllAchievements(int playerId);
    
    // Statistics
    
    /**
     * Get player achievement statistics
     * @param playerId Player ID
     * @return Statistics
     */
    PlayerAchievementStats getPlayerStats(int playerId) const;
    
    /**
     * Get total achievement points earned
     * @param playerId Player ID
     * @return Total points
     */
    int getTotalPoints(int playerId) const;
    
    /**
     * Get completion percentage
     * @param playerId Player ID
     * @return Completion (0-100)
     */
    float getCompletionPercentage(int playerId) const;
    
    /**
     * Get recently unlocked achievements
     * @param playerId Player ID
     * @param count Maximum number to return
     * @return Vector of recently unlocked achievements
     */
    std::vector<Achievement> getRecentlyUnlocked(int playerId, int count = 5) const;
    
    // Persistence
    
    /**
     * Save player achievements to file
     * @param playerId Player ID
     * @param filepath Path to save file
     * @return True if saved
     */
    bool savePlayerAchievements(int playerId, const std::string& filepath);
    
    /**
     * Load player achievements from file
     * @param playerId Player ID
     * @param filepath Path to load file
     * @return True if loaded
     */
    bool loadPlayerAchievements(int playerId, const std::string& filepath);
    
    // Callbacks
    
    /**
     * Set callback for achievement unlocked
     * @param callback Function(event)
     */
    void setAchievementUnlockedCallback(std::function<void(const AchievementUnlockEvent&)> callback);
    
    /**
     * Set callback for achievement progress
     * @param callback Function(event)
     */
    void setAchievementProgressCallback(std::function<void(const AchievementProgressEvent&)> callback);
    
    // Configuration
    
    /**
     * Enable or disable achievement system
     * @param enabled True to enable
     */
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    /**
     * Check if enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * Enable or disable achievement notifications
     * @param enabled True to enable
     */
    void setNotificationsEnabled(bool enabled) { m_notificationsEnabled = enabled; }
    
    /**
     * Set whether to track offline progress
     * @param enabled True to track offline
     */
    void setOfflineTrackingEnabled(bool enabled) { m_offlineTracking = enabled; }
    
    // Utility
    
    /**
     * Get category name
     * @param category Category
     * @return String name
     */
    static const char* getCategoryName(AchievementCategory category);
    
    /**
     * Get condition type name
     * @param type Condition type
     * @return String name
     */
    static const char* getConditionTypeName(ConditionType type);

private:
    // Achievement definitions
    std::unordered_map<std::string, Achievement> m_achievementDefinitions;
    
    // Player progress (playerId -> achievementId -> achievement)
    std::unordered_map<int, std::unordered_map<std::string, Achievement>> m_playerAchievements;
    
    // Callbacks
    std::function<void(const AchievementUnlockEvent&)> m_unlockCallback;
    std::function<void(const AchievementProgressEvent&)> m_progressCallback;
    
    // Configuration
    bool m_enabled;
    bool m_notificationsEnabled;
    bool m_offlineTracking;
    
    // Internal methods
    void updateCondition(int playerId, Achievement& achievement, AchievementCondition& condition, int value);
    void checkAchievementCompletion(int playerId, Achievement& achievement);
    void grantAchievementReward(int playerId, const AchievementReward& reward);
    float calculateProgress(const Achievement& achievement) const;
};

} // namespace Gameplay
} // namespace JJM
