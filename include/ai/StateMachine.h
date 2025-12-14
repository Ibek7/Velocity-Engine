#ifndef JJM_AI_STATEMACHINE_H
#define JJM_AI_STATEMACHINE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

namespace JJM {
namespace AI {

// Forward declarations
class State;
class StateMachine;

/**
 * @brief Transition between states
 */
class StateTransition {
public:
    StateTransition(const std::string& targetState);
    ~StateTransition();

    void setCondition(std::function<bool()> condition);
    bool checkCondition() const;
    std::string getTargetState() const;

private:
    std::string targetState;
    std::function<bool()> condition;
};

/**
 * @brief Base class for state machine states
 */
class State {
public:
    State(const std::string& name);
    virtual ~State();

    virtual void onEnter();
    virtual void onExit();
    virtual void update(float deltaTime);

    void addTransition(std::unique_ptr<StateTransition> transition);
    StateTransition* checkTransitions();

    std::string getName() const;
    void setStateMachine(StateMachine* machine);

protected:
    std::string name;
    StateMachine* stateMachine;
    std::vector<std::unique_ptr<StateTransition>> transitions;
};

/**
 * @brief State machine for managing state transitions
 */
class StateMachine {
public:
    StateMachine();
    ~StateMachine();

    void addState(std::unique_ptr<State> state);
    void setInitialState(const std::string& stateName);
    void changeState(const std::string& stateName);
    void update(float deltaTime);

    State* getCurrentState() const;
    State* getState(const std::string& name);

    void setBlackboardValue(const std::string& key, int value);
    int getBlackboardValue(const std::string& key, int defaultValue = 0) const;

private:
    std::unordered_map<std::string, std::unique_ptr<State>> states;
    State* currentState;
    State* previousState;
    std::unordered_map<std::string, int> blackboard;
};

/**
 * @brief Hierarchical state machine with sub-states
 */
class HierarchicalStateMachine : public State {
public:
    HierarchicalStateMachine(const std::string& name);
    ~HierarchicalStateMachine() override;

    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;

    void addSubState(std::unique_ptr<State> state);
    void setInitialSubState(const std::string& stateName);

private:
    StateMachine subMachine;
};

/**
 * @brief Parallel state machine running multiple states simultaneously
 */
class ParallelStateMachine {
public:
    ParallelStateMachine();
    ~ParallelStateMachine();

    void addStateMachine(std::unique_ptr<StateMachine> machine);
    void update(float deltaTime);

    std::vector<StateMachine*> getActiveMachines();

private:
    std::vector<std::unique_ptr<StateMachine>> machines;
};

/**
 * @brief Pushdown automaton for state stacking
 */
class StateMachineStack {
public:
    StateMachineStack();
    ~StateMachineStack();

    void pushState(std::unique_ptr<State> state);
    void popState();
    void changeState(std::unique_ptr<State> state);
    void update(float deltaTime);

    State* getCurrentState() const;
    size_t getStackSize() const;

private:
    std::vector<std::unique_ptr<State>> stateStack;
};

/**
 * @brief State machine builder for fluent API
 */
class StateMachineBuilder {
public:
    StateMachineBuilder();
    ~StateMachineBuilder();

    StateMachineBuilder& addState(const std::string& name,
                                   std::unique_ptr<State> state);
    
    StateMachineBuilder& addTransition(const std::string& fromState,
                                       const std::string& toState,
                                       std::function<bool()> condition);
    
    StateMachineBuilder& setInitialState(const std::string& stateName);
    
    std::unique_ptr<StateMachine> build();

private:
    std::unique_ptr<StateMachine> machine;
    std::unordered_map<std::string, std::vector<
        std::pair<std::string, std::function<bool()>>>> pendingTransitions;
};

/**
 * @brief AI behavior state for common patterns
 */
class AIBehaviorState : public State {
public:
    AIBehaviorState(const std::string& name);
    ~AIBehaviorState() override;

    void setUpdateCallback(std::function<void(float)> callback);
    void setEnterCallback(std::function<void()> callback);
    void setExitCallback(std::function<void()> callback);

    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;

private:
    std::function<void(float)> updateCallback;
    std::function<void()> enterCallback;
    std::function<void()> exitCallback;
};

} // namespace AI
} // namespace JJM

#endif // JJM_AI_STATEMACHINE_H
