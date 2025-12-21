#ifndef JJM_DIALOGUE_SYSTEM_H
#define JJM_DIALOGUE_SYSTEM_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace JJM {
namespace Gameplay {

struct DialogueOption {
    std::string text;
    std::string nextNodeId;
    std::function<bool()> condition;
};

struct DialogueNode {
    std::string id;
    std::string speaker;
    std::string text;
    std::vector<DialogueOption> options;
    std::function<void()> onEnter;
    std::function<void()> onExit;
};

class DialogueSystem {
public:
    static DialogueSystem& getInstance();
    
    void loadDialogue(const std::string& filename);
    void addNode(const DialogueNode& node);
    
    void startDialogue(const std::string& startNodeId);
    void selectOption(int optionIndex);
    void endDialogue();
    
    bool isActive() const;
    const DialogueNode* getCurrentNode() const;
    std::vector<DialogueOption> getAvailableOptions() const;

private:
    DialogueSystem();
    ~DialogueSystem();
    
    std::map<std::string, DialogueNode> nodes;
    std::string currentNodeId;
    bool active;
};

} // namespace Gameplay
} // namespace JJM

#endif
