#include "gameplay/SkillTree.h"
#include <iostream>

namespace JJM {
namespace Gameplay {

SkillTree::SkillTree() : skillPoints(0) {
}

SkillTree::~SkillTree() {
}

void SkillTree::addNode(const SkillNode& node) {
    nodes[node.id] = node;
}

bool SkillTree::unlockNode(const std::string& nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) {
        std::cerr << "Skill node not found: " << nodeId << std::endl;
        return false;
    }
    
    SkillNode& node = it->second;
    
    if (node.unlocked) {
        std::cout << "Skill already unlocked: " << node.name << std::endl;
        return false;
    }
    
    if (!canUnlockNode(nodeId)) {
        std::cout << "Cannot unlock skill: " << node.name << std::endl;
        return false;
    }
    
    if (skillPoints < node.cost) {
        std::cout << "Not enough skill points for: " << node.name << std::endl;
        return false;
    }
    
    node.unlocked = true;
    skillPoints -= node.cost;
    
    if (node.onUnlock) {
        node.onUnlock();
    }
    
    std::cout << "Unlocked skill: " << node.name << std::endl;
    return true;
}

bool SkillTree::canUnlockNode(const std::string& nodeId) const {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) return false;
    
    const SkillNode& node = it->second;
    
    if (node.unlocked) return false;
    
    for (const auto& reqId : node.requirements) {
        auto reqIt = nodes.find(reqId);
        if (reqIt == nodes.end() || !reqIt->second.unlocked) {
            return false;
        }
    }
    
    return skillPoints >= node.cost;
}

SkillNode* SkillTree::getNode(const std::string& nodeId) {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? &it->second : nullptr;
}

std::vector<SkillNode*> SkillTree::getAllNodes() {
    std::vector<SkillNode*> result;
    for (auto& pair : nodes) {
        result.push_back(&pair.second);
    }
    return result;
}

std::vector<SkillNode*> SkillTree::getUnlockedNodes() {
    std::vector<SkillNode*> result;
    for (auto& pair : nodes) {
        if (pair.second.unlocked) {
            result.push_back(&pair.second);
        }
    }
    return result;
}

std::vector<SkillNode*> SkillTree::getAvailableNodes() const {
    std::vector<SkillNode*> result;
    for (auto& pair : nodes) {
        if (canUnlockNode(pair.first)) {
            result.push_back(const_cast<SkillNode*>(&pair.second));
        }
    }
    return result;
}

int SkillTree::getSkillPoints() const {
    return skillPoints;
}

void SkillTree::addSkillPoints(int points) {
    skillPoints += points;
}

void SkillTree::setSkillPoints(int points) {
    skillPoints = points;
}

float SkillTree::getTotalBonus(const std::string& bonusName) const {
    float total = 0.0f;
    for (const auto& pair : nodes) {
        if (pair.second.unlocked) {
            auto bonusIt = pair.second.bonuses.find(bonusName);
            if (bonusIt != pair.second.bonuses.end()) {
                total += bonusIt->second;
            }
        }
    }
    return total;
}

void SkillTree::reset() {
    int totalSpent = 0;
    for (auto& pair : nodes) {
        if (pair.second.unlocked) {
            totalSpent += pair.second.cost;
            pair.second.unlocked = false;
        }
    }
    skillPoints += totalSpent;
}

} // namespace Gameplay
} // namespace JJM
