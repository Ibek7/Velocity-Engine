#include "state/StateMachine.h"
#include <iostream>

namespace JJM {
namespace State {

// State implementation
State::State(const std::string& stateName) : name(stateName), active(false) {}

State::~State() {}

// StateMachine implementation
StateMachine::StateMachine() : currentState(nullptr) {}

StateMachine::~StateMachine() {
    if (currentState) {
        currentState->onExit();
    }
}

void StateMachine::addState(std::unique_ptr<State> state) {
    std::string stateName = state->getName();
    states[stateName] = std::move(state);
}

void StateMachine::removeState(const std::string& stateName) {
    if (currentStateName == stateName) {
        if (currentState) {
            currentState->onExit();
            currentState = nullptr;
            currentStateName.clear();
        }
    }
    states.erase(stateName);
}

void StateMachine::changeState(const std::string& stateName) {
    auto it = states.find(stateName);
    if (it == states.end()) {
        std::cerr << "State not found: " << stateName << std::endl;
        return;
    }
    
    // Exit current state
    if (currentState) {
        currentState->onExit();
        currentState->setActive(false);
    }
    
    // Enter new state
    currentStateName = stateName;
    currentState = it->second.get();
    currentState->setActive(true);
    currentState->onEnter();
}

void StateMachine::update(float deltaTime) {
    if (currentState && currentState->isActive()) {
        currentState->update(deltaTime);
    }
}

void StateMachine::render() {
    if (currentState && currentState->isActive()) {
        currentState->render();
    }
}

bool StateMachine::hasState(const std::string& stateName) const {
    return states.find(stateName) != states.end();
}

// MenuState implementation
MenuState::MenuState() : State("Menu") {}

void MenuState::onEnter() {
    std::cout << "Entering Menu State" << std::endl;
}

void MenuState::onExit() {
    std::cout << "Exiting Menu State" << std::endl;
}

void MenuState::update(float deltaTime) {
    // Menu update logic
}

void MenuState::render() {
    // Menu render logic
}

// GameState implementation
GameState::GameState() : State("Game") {}

void GameState::onEnter() {
    std::cout << "Entering Game State" << std::endl;
}

void GameState::onExit() {
    std::cout << "Exiting Game State" << std::endl;
}

void GameState::update(float deltaTime) {
    // Game update logic
}

void GameState::render() {
    // Game render logic
}

// PausedState implementation
PausedState::PausedState() : State("Paused") {}

void PausedState::onEnter() {
    std::cout << "Entering Paused State" << std::endl;
}

void PausedState::onExit() {
    std::cout << "Exiting Paused State" << std::endl;
}

void PausedState::update(float deltaTime) {
    // Paused update logic
}

void PausedState::render() {
    // Paused render logic
}

// GameOverState implementation
GameOverState::GameOverState() : State("GameOver") {}

void GameOverState::onEnter() {
    std::cout << "Entering Game Over State" << std::endl;
}

void GameOverState::onExit() {
    std::cout << "Exiting Game Over State" << std::endl;
}

void GameOverState::update(float deltaTime) {
    // Game over update logic
}

void GameOverState::render() {
    // Game over render logic
}

} // namespace State
} // namespace JJM
