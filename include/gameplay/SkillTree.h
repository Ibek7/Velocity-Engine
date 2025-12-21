#ifndef JJM_SKILL_TREE_H
#define JJM_SKILL_TREE_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace JJM {
namespace Gameplay {

struct SkillNode {
    std::string id;
    std::string name;
    std::string description;
    int tier;
    int cost;
    
    std::vector<std::string> requirements;
    bool unlocked;
    
    std::map<std::string, float> bonuses;
    std::function<void()> onUnlock;
};

class SkillTree {
public:
    SkillTree();
    ~SkillTree();
    
    void addNode(const SkillNode& node);
    bool unlockNode(const std::string& nodeId);
    bool canUnlockNode(const std::string& nodeId) const;
    
    SkillNode* getNode(const std::string& nodeId);
    std::vector<SkillNode*> getAllNodes();
    std::vector<SkillNode*> getUnlockedNodes();
    std::vector<SkillNode*> getAvailableNodes() const;
    
    int getSkillPoints() const;
    void addSkillPoints(int points);
    void setSkillPoints(int points);
    
    float getTotalBonus(const std::string& bonusName) const;
    
    void reset();

private:
    std::map<std::string, SkillNode> nodes;
    int skillPoints;
};

} // namespace Gameplay
} // namespace JJM

#endif
