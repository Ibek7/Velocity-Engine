#ifndef ACHIEVEMENT_SYSTEM_H
#define ACHIEVEMENT_SYSTEM_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace JJM {
namespace Achievements {

enum class AchievementType {
    OneTime,
    Progress,
    Tiered
};

struct Achievement {
    std::string id;
    std::string name;
    std::string description;
    AchievementType type;
    int targetValue;
    int currentValue;
    bool unlocked;
    std::string iconPath;
    int points;
    
    Achievement() : type(AchievementType::OneTime), targetValue(1), currentValue(0), unlocked(false), points(10) {}
    Achievement(const std::string& id, const std::string& name, const std::string& desc,
                AchievementType type = AchievementType::OneTime, int target = 1)
        : id(id), name(name), description(desc), type(type), targetValue(target),
          currentValue(0), unlocked(false), points(10) {}
};

class AchievementSystem {
public:
    static AchievementSystem& getInstance();
    
    void registerAchievement(const Achievement& achievement);
    void incrementProgress(const std::string& id, int amount = 1);
    void setProgress(const std::string& id, int value);
    bool unlock(const std::string& id);
    
    bool isUnlocked(const std::string& id) const;
    int getProgress(const std::string& id) const;
    float getProgressPercent(const std::string& id) const;
    
    std::vector<Achievement> getAllAchievements() const;
    std::vector<Achievement> getUnlockedAchievements() const;
    std::vector<Achievement> getLockedAchievements() const;
    
    void setOnUnlock(std::function<void(const Achievement&)> callback) { onUnlockCallback = callback; }
    void setOnProgress(std::function<void(const Achievement&, int)> callback) { onProgressCallback = callback; }
    
    int getTotalPoints() const;
    int getEarnedPoints() const;
    
    void save();
    void load();
    
private:
    AchievementSystem() = default;
    std::unordered_map<std::string, Achievement> achievements;
    std::function<void(const Achievement&)> onUnlockCallback;
    std::function<void(const Achievement&, int)> onProgressCallback;
    
    void checkAndUnlock(Achievement& achievement);
};

} // namespace Achievements
} // namespace JJM

#endif
