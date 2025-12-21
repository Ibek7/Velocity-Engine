#ifndef JJM_GAME_RULES_H
#define JJM_GAME_RULES_H

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace JJM {
namespace Gameplay {

using RuleCallback = std::function<bool()>;

struct GameRule {
    std::string name;
    std::string description;
    RuleCallback condition;
    RuleCallback action;
    bool enabled;
    int priority;
};

class GameRulesEngine {
public:
    static GameRulesEngine& getInstance();
    
    void registerRule(const std::string& name, const std::string& desc,
                     RuleCallback condition, RuleCallback action, int priority = 0);
    
    void enableRule(const std::string& name);
    void disableRule(const std::string& name);
    bool isRuleEnabled(const std::string& name) const;
    
    void evaluateRules();
    void evaluateRule(const std::string& name);
    
    std::vector<GameRule*> getAllRules();
    GameRule* getRule(const std::string& name);

private:
    GameRulesEngine();
    ~GameRulesEngine();
    
    std::map<std::string, GameRule> rules;
};

} // namespace Gameplay
} // namespace JJM

#endif
