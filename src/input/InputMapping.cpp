#include "input/InputMapping.h"
#include <fstream>
#include <sstream>

namespace JJM {
namespace Input {

// InputAction implementation
InputAction::InputAction(const std::string& name)
    : name(name), pressed(false), justPressed(false),
      justReleased(false), value(0.0f), previousState(false) {}

InputAction::~InputAction() {}

void InputAction::addBinding(const InputBinding& binding) {
    bindings.push_back(binding);
}

void InputAction::removeBinding(size_t index) {
    if (index < bindings.size()) {
        bindings.erase(bindings.begin() + index);
    }
}

void InputAction::clearBindings() {
    bindings.clear();
}

void InputAction::update(bool currentState, float currentValue) {
    previousState = pressed;
    pressed = currentState;
    value = currentValue;
    
    justPressed = pressed && !previousState;
    justReleased = !pressed && previousState;
}

// InputMap implementation
InputMap::InputMap(const std::string& name) : name(name) {}

InputMap::~InputMap() {}

void InputMap::addAction(const std::string& actionName) {
    actions.emplace(actionName, InputAction(actionName));
}

void InputMap::removeAction(const std::string& actionName) {
    actions.erase(actionName);
}

InputAction* InputMap::getAction(const std::string& actionName) {
    auto it = actions.find(actionName);
    if (it != actions.end()) {
        return &it->second;
    }
    return nullptr;
}

void InputMap::bindKey(const std::string& actionName, int keyCode) {
    auto* action = getAction(actionName);
    if (action) {
        InputBinding binding;
        binding.action = actionName;
        binding.type = InputType::Keyboard;
        binding.keyCode = keyCode;
        action->addBinding(binding);
    }
}

void InputMap::bindMouseButton(const std::string& actionName, int button) {
    auto* action = getAction(actionName);
    if (action) {
        InputBinding binding;
        binding.action = actionName;
        binding.type = InputType::Mouse;
        binding.mouseButton = button;
        action->addBinding(binding);
    }
}

void InputMap::bindGamepadButton(const std::string& actionName, int button) {
    auto* action = getAction(actionName);
    if (action) {
        InputBinding binding;
        binding.action = actionName;
        binding.type = InputType::Gamepad;
        binding.gamepadButton = button;
        action->addBinding(binding);
    }
}

void InputMap::unbindKey(const std::string& actionName, int keyCode) {
    auto* action = getAction(actionName);
    if (action) {
        auto& bindings = action->getBindings();
        for (size_t i = 0; i < bindings.size(); ++i) {
            if (bindings[i].type == InputType::Keyboard && 
                bindings[i].keyCode == keyCode) {
                action->removeBinding(i);
                break;
            }
        }
    }
}

bool InputMap::isActionPressed(const std::string& actionName) const {
    auto it = actions.find(actionName);
    return it != actions.end() && it->second.isPressed();
}

bool InputMap::isActionJustPressed(const std::string& actionName) const {
    auto it = actions.find(actionName);
    return it != actions.end() && it->second.wasJustPressed();
}

bool InputMap::isActionJustReleased(const std::string& actionName) const {
    auto it = actions.find(actionName);
    return it != actions.end() && it->second.wasJustReleased();
}

float InputMap::getActionValue(const std::string& actionName) const {
    auto it = actions.find(actionName);
    return it != actions.end() ? it->second.getValue() : 0.0f;
}

void InputMap::update() {
    for (auto& pair : actions) {
        pair.second.update(false, 0.0f);
    }
}

// InputMapManager implementation
InputMapManager::InputMapManager() : activeMap(nullptr) {}

InputMapManager::~InputMapManager() {}

void InputMapManager::createInputMap(const std::string& name) {
    inputMaps.emplace(name, InputMap(name));
}

InputMap* InputMapManager::getInputMap(const std::string& name) {
    auto it = inputMaps.find(name);
    if (it != inputMaps.end()) {
        return &it->second;
    }
    return nullptr;
}

void InputMapManager::setActiveMap(const std::string& name) {
    activeMap = getInputMap(name);
}

void InputMapManager::pushInputMap(const std::string& name) {
    auto* map = getInputMap(name);
    if (map) {
        inputMapStack.push_back(map);
        activeMap = map;
    }
}

void InputMapManager::popInputMap() {
    if (!inputMapStack.empty()) {
        inputMapStack.pop_back();
        activeMap = inputMapStack.empty() ? nullptr : inputMapStack.back();
    }
}

void InputMapManager::update() {
    if (activeMap) {
        activeMap->update();
    }
}

// InputRebinding implementation
InputRebinding::InputRebinding()
    : rebinding(false), rebindingAction(""), callback(nullptr) {}

InputRebinding::~InputRebinding() {}

void InputRebinding::startRebinding(const std::string& actionName) {
    rebinding = true;
    rebindingAction = actionName;
}

void InputRebinding::cancelRebinding() {
    rebinding = false;
    rebindingAction = "";
}

void InputRebinding::setKeyBinding(int keyCode) {
    if (!rebinding) return;
    
    InputBinding binding;
    binding.action = rebindingAction;
    binding.type = InputType::Keyboard;
    binding.keyCode = keyCode;
    
    if (callback) {
        callback(rebindingAction, binding);
    }
    
    rebinding = false;
    rebindingAction = "";
}

void InputRebinding::setMouseButtonBinding(int button) {
    if (!rebinding) return;
    
    InputBinding binding;
    binding.action = rebindingAction;
    binding.type = InputType::Mouse;
    binding.mouseButton = button;
    
    if (callback) {
        callback(rebindingAction, binding);
    }
    
    rebinding = false;
    rebindingAction = "";
}

void InputRebinding::setGamepadButtonBinding(int button) {
    if (!rebinding) return;
    
    InputBinding binding;
    binding.action = rebindingAction;
    binding.type = InputType::Gamepad;
    binding.gamepadButton = button;
    
    if (callback) {
        callback(rebindingAction, binding);
    }
    
    rebinding = false;
    rebindingAction = "";
}

void InputRebinding::saveBindings(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "# Input Bindings\n";
    file.close();
}

void InputRebinding::loadBindings(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        // Parse bindings
    }
}

// InputContext implementation
InputContext::InputContext(const std::string& name)
    : name(name), enabled(true), priority(0) {}

InputContext::~InputContext() {}

void InputContext::addAction(const std::string& actionName) {
    actions.emplace(actionName, InputAction(actionName));
}

InputAction* InputContext::getAction(const std::string& actionName) {
    auto it = actions.find(actionName);
    if (it != actions.end()) {
        return &it->second;
    }
    return nullptr;
}

// InputProfile implementation
InputProfile::InputProfile(const std::string& name) : name(name) {}

InputProfile::~InputProfile() {}

void InputProfile::addBinding(const std::string& actionName, const InputBinding& binding) {
    bindings[actionName].push_back(binding);
}

void InputProfile::removeBinding(const std::string& actionName, size_t index) {
    auto it = bindings.find(actionName);
    if (it != bindings.end() && index < it->second.size()) {
        it->second.erase(it->second.begin() + index);
    }
}

std::vector<InputBinding> InputProfile::getBindings(const std::string& actionName) const {
    auto it = bindings.find(actionName);
    if (it != bindings.end()) {
        return it->second;
    }
    return std::vector<InputBinding>();
}

void InputProfile::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return;
    
    file << "# Input Profile: " << name << "\n";
    
    for (const auto& pair : bindings) {
        file << "Action: " << pair.first << "\n";
        for (const auto& binding : pair.second) {
            file << "  Binding: " << static_cast<int>(binding.type) 
                 << " " << binding.keyCode << "\n";
        }
    }
    
    file.close();
}

void InputProfile::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        // Parse profile
    }
}

} // namespace Input
} // namespace JJM
