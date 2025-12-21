#include "gameplay/GameRules.h"
#include <algorithm>

namespace JJM {
namespace Gameplay {

GameRulesEngine& GameRulesEngine::getInstance() {
    static GameRulesEngine instance;
    return instance;
}

GameRulesEngine::GameRulesEngine() {
}

GameRulesEngine::~GameRulesEngine() {
}

void GameRulesEngine::registerRule(const std::string& name, const std::string& desc,
                                  RuleCallback condition, RuleCallback action, int priority) {
    GameRule rule;
    rule.name = name;
    rule.description = desc;
    rule.condition = condition;
    rule.action = action;
    rule.enabled = true;
    rule.priority = priority;
    
    rules[name] = rule;
}

void GameRulesEngine::enableRule(const std::string& name) {
    auto it = rules.find(name);
    if (it != rules.end()) {
        it->second.enabled = true;
    }
}

void GameRulesEngine::disableRule(const std::string& name) {
    auto it = rules.find(name);
    if (it != rules.end()) {
        it->second.enabled = false;
    }
}

bool GameRulesEngine::isRuleEnabled(const std::string& name) const {
    auto it = rules.find(name);
    return it != rules.end() && it->second.enabled;
}

void GameRulesEngine::evaluateRules() {
    std::vector<GameRule*> sortedRules;
    for (auto& pair : rules) {
        if (pair.second.enabled) {
            sortedRules.push_back(&pair.second);
        }
    }
    
    std::sort(sortedRules.begin(), sortedRules.end(),
        [](const GameRule* a, const GameRule* b) {
            return a->priority > b->priority;
        });
    
    for (auto* rule : sortedRules) {
        if (rule->condition && rule->condition()) {
            if (rule->action) {
                rule->action();
            }
        }
    }
}

void GameRulesEngine::evaluateRule(const std::string& name) {
    auto it = rules.find(name);
    if (it != rules.end() && it->second.enabled) {
        if (it->second.condition && it->second.condition()) {
            if (it->second.action) {
                it->second.action();
            }
        }
    }
}

std::vector<GameRule*> GameRulesEngine::getAllRules() {
    std::vector<GameRule*> result;
    for (auto& pair : rules) {
        result.push_back(&pair.second);
    }
    return result;
}

GameRule* GameRulesEngine::getRule(const std::string& name) {
    auto it = rules.find(name);
    return it != rules.end() ? &it->second : nullptr;
}

} // namespace Gameplay
} // namespace JJM
