#include "state/AchievementSystem.h"
#include <iostream>
#include <algorithm>

namespace JJM {
namespace Achievements {

AchievementSystem& AchievementSystem::getInstance() {
    static AchievementSystem instance;
    return instance;
}

void AchievementSystem::registerAchievement(const Achievement& achievement) {
    achievements[achievement.id] = achievement;
}

void AchievementSystem::incrementProgress(const std::string& id, int amount) {
    auto it = achievements.find(id);
    if (it == achievements.end()) return;
    
    Achievement& achievement = it->second;
    if (achievement.unlocked) return;
    
    achievement.currentValue += amount;
    
    if (onProgressCallback) {
        onProgressCallback(achievement, achievement.currentValue);
    }
    
    checkAndUnlock(achievement);
}

void AchievementSystem::setProgress(const std::string& id, int value) {
    auto it = achievements.find(id);
    if (it == achievements.end()) return;
    
    Achievement& achievement = it->second;
    if (achievement.unlocked) return;
    
    achievement.currentValue = value;
    
    if (onProgressCallback) {
        onProgressCallback(achievement, achievement.currentValue);
    }
    
    checkAndUnlock(achievement);
}

bool AchievementSystem::unlock(const std::string& id) {
    auto it = achievements.find(id);
    if (it == achievements.end()) return false;
    
    Achievement& achievement = it->second;
    if (achievement.unlocked) return false;
    
    achievement.unlocked = true;
    achievement.currentValue = achievement.targetValue;
    
    std::cout << "Achievement Unlocked: " << achievement.name << std::endl;
    
    if (onUnlockCallback) {
        onUnlockCallback(achievement);
    }
    
    return true;
}

void AchievementSystem::checkAndUnlock(Achievement& achievement) {
    if (achievement.currentValue >= achievement.targetValue) {
        unlock(achievement.id);
    }
}

bool AchievementSystem::isUnlocked(const std::string& id) const {
    auto it = achievements.find(id);
    return it != achievements.end() && it->second.unlocked;
}

int AchievementSystem::getProgress(const std::string& id) const {
    auto it = achievements.find(id);
    return it != achievements.end() ? it->second.currentValue : 0;
}

float AchievementSystem::getProgressPercent(const std::string& id) const {
    auto it = achievements.find(id);
    if (it == achievements.end() || it->second.targetValue == 0) return 0.0f;
    
    return static_cast<float>(it->second.currentValue) / static_cast<float>(it->second.targetValue) * 100.0f;
}

std::vector<Achievement> AchievementSystem::getAllAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, achievement] : achievements) {
        result.push_back(achievement);
    }
    return result;
}

std::vector<Achievement> AchievementSystem::getUnlockedAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, achievement] : achievements) {
        if (achievement.unlocked) {
            result.push_back(achievement);
        }
    }
    return result;
}

std::vector<Achievement> AchievementSystem::getLockedAchievements() const {
    std::vector<Achievement> result;
    for (const auto& [id, achievement] : achievements) {
        if (!achievement.unlocked) {
            result.push_back(achievement);
        }
    }
    return result;
}

int AchievementSystem::getTotalPoints() const {
    int total = 0;
    for (const auto& [id, achievement] : achievements) {
        total += achievement.points;
    }
    return total;
}

int AchievementSystem::getEarnedPoints() const {
    int earned = 0;
    for (const auto& [id, achievement] : achievements) {
        if (achievement.unlocked) {
            earned += achievement.points;
        }
    }
    return earned;
}

void AchievementSystem::save() {
    // Would save to file
    std::cout << "Saving achievements..." << std::endl;
}

void AchievementSystem::load() {
    // Would load from file
    std::cout << "Loading achievements..." << std::endl;
}

} // namespace Achievements
} // namespace JJM
