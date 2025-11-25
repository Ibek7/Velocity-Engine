#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

namespace JJM {
namespace State {

class State {
protected:
    std::string name;
    bool active;
    
public:
    State(const std::string& stateName);
    virtual ~State();
    
    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float deltaTime) {}
    virtual void render() {}
    
    const std::string& getName() const { return name; }
    bool isActive() const { return active; }
    void setActive(bool act) { active = act; }
};

class StateMachine {
private:
    std::unordered_map<std::string, std::unique_ptr<State>> states;
    State* currentState;
    std::string currentStateName;
    
public:
    StateMachine();
    ~StateMachine();
    
    void addState(std::unique_ptr<State> state);
    void removeState(const std::string& stateName);
    
    void changeState(const std::string& stateName);
    void update(float deltaTime);
    void render();
    
    State* getCurrentState() const { return currentState; }
    const std::string& getCurrentStateName() const { return currentStateName; }
    
    bool hasState(const std::string& stateName) const;
};

// Common game states
class MenuState : public State {
public:
    MenuState();
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render() override;
};

class GameState : public State {
public:
    GameState();
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render() override;
};

class PausedState : public State {
public:
    PausedState();
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render() override;
};

class GameOverState : public State {
public:
    GameOverState();
    void onEnter() override;
    void onExit() override;
    void update(float deltaTime) override;
    void render() override;
};

} // namespace State
} // namespace JJM

#endif // STATE_MACHINE_H
