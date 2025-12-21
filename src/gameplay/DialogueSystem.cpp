#include "gameplay/DialogueSystem.h"
#include <iostream>

namespace JJM {
namespace Gameplay {

DialogueSystem& DialogueSystem::getInstance() {
    static DialogueSystem instance;
    return instance;
}

DialogueSystem::DialogueSystem() : active(false) {
}

DialogueSystem::~DialogueSystem() {
}

void DialogueSystem::loadDialogue(const std::string& filename) {
    std::cout << "Loading dialogue from: " << filename << std::endl;
}

void DialogueSystem::addNode(const DialogueNode& node) {
    nodes[node.id] = node;
}

void DialogueSystem::startDialogue(const std::string& startNodeId) {
    auto it = nodes.find(startNodeId);
    if (it == nodes.end()) {
        std::cerr << "Dialogue node not found: " << startNodeId << std::endl;
        return;
    }
    
    currentNodeId = startNodeId;
    active = true;
    
    if (it->second.onEnter) {
        it->second.onEnter();
    }
}

void DialogueSystem::selectOption(int optionIndex) {
    if (!active) return;
    
    auto currentNode = nodes.find(currentNodeId);
    if (currentNode == nodes.end()) return;
    
    const auto& options = getAvailableOptions();
    if (optionIndex < 0 || optionIndex >= static_cast<int>(options.size())) {
        return;
    }
    
    if (currentNode->second.onExit) {
        currentNode->second.onExit();
    }
    
    const std::string& nextId = options[optionIndex].nextNodeId;
    if (nextId.empty() || nextId == "END") {
        endDialogue();
        return;
    }
    
    currentNodeId = nextId;
    auto nextNode = nodes.find(currentNodeId);
    if (nextNode != nodes.end() && nextNode->second.onEnter) {
        nextNode->second.onEnter();
    }
}

void DialogueSystem::endDialogue() {
    if (!active) return;
    
    auto currentNode = nodes.find(currentNodeId);
    if (currentNode != nodes.end() && currentNode->second.onExit) {
        currentNode->second.onExit();
    }
    
    active = false;
    currentNodeId.clear();
}

bool DialogueSystem::isActive() const {
    return active;
}

const DialogueNode* DialogueSystem::getCurrentNode() const {
    if (!active) return nullptr;
    
    auto it = nodes.find(currentNodeId);
    return it != nodes.end() ? &it->second : nullptr;
}

std::vector<DialogueOption> DialogueSystem::getAvailableOptions() const {
    std::vector<DialogueOption> availableOptions;
    
    const DialogueNode* node = getCurrentNode();
    if (!node) return availableOptions;
    
    for (const auto& option : node->options) {
        if (!option.condition || option.condition()) {
            availableOptions.push_back(option);
        }
    }
    
    return availableOptions;
}

} // namespace Gameplay
} // namespace JJM
