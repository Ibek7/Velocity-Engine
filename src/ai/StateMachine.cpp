#include "ai/StateMachine.h"

namespace JJM {
namespace AI {

// StateTransition implementation
StateTransition::StateTransition(const std::string& targetState)
    : targetState(targetState) {}

StateTransition::~StateTransition() {}

void StateTransition::setCondition(std::function<bool()> condition) {
    this->condition = condition;
}

bool StateTransition::checkCondition() const {
    return condition ? condition() : true;
}

std::string StateTransition::getTargetState() const {
    return targetState;
}

// State implementation
State::State(const std::string& name)
    : name(name), stateMachine(nullptr) {}

State::~State() {}

void State::onEnter() {
    // Default implementation
}

void State::onExit() {
    // Default implementation
}

void State::update(float deltaTime) {
    (void)deltaTime; // Unused
}

void State::addTransition(std::unique_ptr<StateTransition> transition) {
    transitions.push_back(std::move(transition));
}

StateTransition* State::checkTransitions() {
    for (auto& transition : transitions) {
        if (transition->checkCondition()) {
            return transition.get();
        }
    }
    return nullptr;
}

std::string State::getName() const {
    return name;
}

void State::setStateMachine(StateMachine* machine) {
    this->stateMachine = machine;
}

// StateMachine implementation
StateMachine::StateMachine()
    : currentState(nullptr), previousState(nullptr) {}

StateMachine::~StateMachine() {}

void StateMachine::addState(std::unique_ptr<State> state) {
    std::string name = state->getName();
    state->setStateMachine(this);
    states[name] = std::move(state);
}

void StateMachine::setInitialState(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it != states.end()) {
        currentState = it->second.get();
        currentState->onEnter();
    }
}

void StateMachine::changeState(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it == states.end() || it->second.get() == currentState) {
        return;
    }

    if (currentState) {
        currentState->onExit();
    }

    previousState = currentState;
    currentState = it->second.get();
    currentState->onEnter();
}

void StateMachine::update(float deltaTime) {
    if (!currentState) return;

    currentState->update(deltaTime);

    // Check for transitions
    StateTransition* transition = currentState->checkTransitions();
    if (transition) {
        changeState(transition->getTargetState());
    }
}

State* StateMachine::getCurrentState() const {
    return currentState;
}

State* StateMachine::getState(const std::string& name) {
    auto it = states.find(name);
    return it != states.end() ? it->second.get() : nullptr;
}

void StateMachine::setBlackboardValue(const std::string& key, int value) {
    blackboard[key] = value;
}

int StateMachine::getBlackboardValue(const std::string& key, int defaultValue) const {
    auto it = blackboard.find(key);
    return it != blackboard.end() ? it->second : defaultValue;
}

// HierarchicalStateMachine implementation
HierarchicalStateMachine::HierarchicalStateMachine(const std::string& name)
    : State(name) {}

HierarchicalStateMachine::~HierarchicalStateMachine() {}

void HierarchicalStateMachine::onEnter() {
    State::onEnter();
    if (subMachine.getCurrentState()) {
        subMachine.getCurrentState()->onEnter();
    }
}

void HierarchicalStateMachine::onExit() {
    if (subMachine.getCurrentState()) {
        subMachine.getCurrentState()->onExit();
    }
    State::onExit();
}

void HierarchicalStateMachine::update(float deltaTime) {
    subMachine.update(deltaTime);
}

void HierarchicalStateMachine::addSubState(std::unique_ptr<State> state) {
    subMachine.addState(std::move(state));
}

void HierarchicalStateMachine::setInitialSubState(const std::string& stateName) {
    subMachine.setInitialState(stateName);
}

// ParallelStateMachine implementation
ParallelStateMachine::ParallelStateMachine() {}

ParallelStateMachine::~ParallelStateMachine() {}

void ParallelStateMachine::addStateMachine(std::unique_ptr<StateMachine> machine) {
    machines.push_back(std::move(machine));
}

void ParallelStateMachine::update(float deltaTime) {
    for (auto& machine : machines) {
        machine->update(deltaTime);
    }
}

std::vector<StateMachine*> ParallelStateMachine::getActiveMachines() {
    std::vector<StateMachine*> active;
    for (auto& machine : machines) {
        if (machine->getCurrentState()) {
            active.push_back(machine.get());
        }
    }
    return active;
}

// StateMachineStack implementation
StateMachineStack::StateMachineStack() {}

StateMachineStack::~StateMachineStack() {}

void StateMachineStack::pushState(std::unique_ptr<State> state) {
    if (!stateStack.empty()) {
        stateStack.back()->onExit();
    }
    stateStack.push_back(std::move(state));
    stateStack.back()->onEnter();
}

void StateMachineStack::popState() {
    if (stateStack.empty()) return;

    stateStack.back()->onExit();
    stateStack.pop_back();

    if (!stateStack.empty()) {
        stateStack.back()->onEnter();
    }
}

void StateMachineStack::changeState(std::unique_ptr<State> state) {
    if (!stateStack.empty()) {
        stateStack.back()->onExit();
        stateStack.pop_back();
    }
    stateStack.push_back(std::move(state));
    stateStack.back()->onEnter();
}

void StateMachineStack::update(float deltaTime) {
    if (!stateStack.empty()) {
        stateStack.back()->update(deltaTime);
    }
}

State* StateMachineStack::getCurrentState() const {
    return stateStack.empty() ? nullptr : stateStack.back().get();
}

size_t StateMachineStack::getStackSize() const {
    return stateStack.size();
}

// StateMachineBuilder implementation
StateMachineBuilder::StateMachineBuilder()
    : machine(std::make_unique<StateMachine>()) {}

StateMachineBuilder::~StateMachineBuilder() {}

StateMachineBuilder& StateMachineBuilder::addState(const std::string& name,
                                                    std::unique_ptr<State> state) {
    machine->addState(std::move(state));
    return *this;
}

StateMachineBuilder& StateMachineBuilder::addTransition(
    const std::string& fromState,
    const std::string& toState,
    std::function<bool()> condition) {
    
    pendingTransitions[fromState].push_back({toState, condition});
    return *this;
}

StateMachineBuilder& StateMachineBuilder::setInitialState(const std::string& stateName) {
    machine->setInitialState(stateName);
    return *this;
}

std::unique_ptr<StateMachine> StateMachineBuilder::build() {
    // Apply pending transitions
    for (const auto& pair : pendingTransitions) {
        State* fromState = machine->getState(pair.first);
        if (fromState) {
            for (const auto& trans : pair.second) {
                auto transition = std::make_unique<StateTransition>(trans.first);
                transition->setCondition(trans.second);
                fromState->addTransition(std::move(transition));
            }
        }
    }

    return std::move(machine);
}

// AIBehaviorState implementation
AIBehaviorState::AIBehaviorState(const std::string& name)
    : State(name) {}

AIBehaviorState::~AIBehaviorState() {}

void AIBehaviorState::setUpdateCallback(std::function<void(float)> callback) {
    this->updateCallback = callback;
}

void AIBehaviorState::setEnterCallback(std::function<void()> callback) {
    this->enterCallback = callback;
}

void AIBehaviorState::setExitCallback(std::function<void()> callback) {
    this->exitCallback = callback;
}

void AIBehaviorState::onEnter() {
    State::onEnter();
    if (enterCallback) {
        enterCallback();
    }
}

void AIBehaviorState::onExit() {
    if (exitCallback) {
        exitCallback();
    }
    State::onExit();
}

void AIBehaviorState::update(float deltaTime) {
    if (updateCallback) {
        updateCallback(deltaTime);
    }
}

} // namespace AI
} // namespace JJM
