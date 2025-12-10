#pragma once

#include "input/InputManager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace Input {

enum class InputType {
    Keyboard,
    Mouse,
    Gamepad,
    Touch
};

struct InputBinding {
    std::string action;
    InputType type;
    int keyCode;
    int mouseButton;
    int gamepadButton;
    float deadzone;
    bool isAxis;
    
    InputBinding() : action(""), type(InputType::Keyboard),
                     keyCode(0), mouseButton(0), gamepadButton(0),
                     deadzone(0.1f), isAxis(false) {}
};

class InputAction {
public:
    InputAction(const std::string& name);
    ~InputAction();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void addBinding(const InputBinding& binding);
    void removeBinding(size_t index);
    void clearBindings();
    
    const std::vector<InputBinding>& getBindings() const { return bindings; }
    
    bool isPressed() const { return pressed; }
    bool wasJustPressed() const { return justPressed; }
    bool wasJustReleased() const { return justReleased; }
    
    float getValue() const { return value; }
    
    void update(bool currentState, float currentValue = 0.0f);

private:
    std::string name;
    std::vector<InputBinding> bindings;
    bool pressed;
    bool justPressed;
    bool justReleased;
    float value;
    bool previousState;
};

class InputMap {
public:
    InputMap(const std::string& name);
    ~InputMap();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void addAction(const std::string& actionName);
    void removeAction(const std::string& actionName);
    
    InputAction* getAction(const std::string& actionName);
    
    void bindKey(const std::string& actionName, int keyCode);
    void bindMouseButton(const std::string& actionName, int button);
    void bindGamepadButton(const std::string& actionName, int button);
    
    void unbindKey(const std::string& actionName, int keyCode);
    
    bool isActionPressed(const std::string& actionName) const;
    bool isActionJustPressed(const std::string& actionName) const;
    bool isActionJustReleased(const std::string& actionName) const;
    
    float getActionValue(const std::string& actionName) const;
    
    void update();

private:
    std::string name;
    std::unordered_map<std::string, InputAction> actions;
};

class InputMapManager {
public:
    InputMapManager();
    ~InputMapManager();
    
    void createInputMap(const std::string& name);
    InputMap* getInputMap(const std::string& name);
    
    void setActiveMap(const std::string& name);
    InputMap* getActiveMap() { return activeMap; }
    
    void pushInputMap(const std::string& name);
    void popInputMap();
    
    void update();

private:
    std::unordered_map<std::string, InputMap> inputMaps;
    InputMap* activeMap;
    std::vector<InputMap*> inputMapStack;
};

class InputRebinding {
public:
    InputRebinding();
    ~InputRebinding();
    
    void startRebinding(const std::string& actionName);
    void cancelRebinding();
    
    bool isRebinding() const { return rebinding; }
    const std::string& getRebindingAction() const { return rebindingAction; }
    
    void setKeyBinding(int keyCode);
    void setMouseButtonBinding(int button);
    void setGamepadButtonBinding(int button);
    
    void saveBindings(const std::string& filepath);
    void loadBindings(const std::string& filepath);
    
    using RebindCallback = std::function<void(const std::string&, const InputBinding&)>;
    void setRebindCallback(RebindCallback callback) { this->callback = callback; }

private:
    bool rebinding;
    std::string rebindingAction;
    RebindCallback callback;
};

class InputContext {
public:
    InputContext(const std::string& name);
    ~InputContext();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setPriority(int priority) { this->priority = priority; }
    int getPriority() const { return priority; }
    
    void addAction(const std::string& actionName);
    InputAction* getAction(const std::string& actionName);

private:
    std::string name;
    bool enabled;
    int priority;
    std::unordered_map<std::string, InputAction> actions;
};

class InputProfile {
public:
    InputProfile(const std::string& name);
    ~InputProfile();
    
    void setName(const std::string& name) { this->name = name; }
    const std::string& getName() const { return name; }
    
    void addBinding(const std::string& actionName, const InputBinding& binding);
    void removeBinding(const std::string& actionName, size_t index);
    
    std::vector<InputBinding> getBindings(const std::string& actionName) const;
    
    void save(const std::string& filepath);
    void load(const std::string& filepath);

private:
    std::string name;
    std::unordered_map<std::string, std::vector<InputBinding>> bindings;
};

} // namespace Input
} // namespace JJM
