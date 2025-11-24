#include "scene/SceneManager.h"
#include <iostream>

namespace JJM {
namespace Scene {

SceneManager::SceneManager() : shouldChange(false) {}

SceneManager::~SceneManager() {
    while (!sceneStack.empty()) {
        sceneStack.pop();
    }
    scenes.clear();
}

void SceneManager::addScene(const std::string& name, std::shared_ptr<Scene> scene) {
    scenes[name] = scene;
}

void SceneManager::removeScene(const std::string& name) {
    scenes.erase(name);
}

void SceneManager::changeScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        nextScene = it->second;
        shouldChange = true;
    } else {
        std::cerr << "Scene not found: " << name << std::endl;
    }
}

void SceneManager::pushScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        if (!sceneStack.empty()) {
            sceneStack.top()->onExit();
        }
        sceneStack.push(it->second);
        sceneStack.top()->onEnter();
    }
}

void SceneManager::popScene() {
    if (!sceneStack.empty()) {
        sceneStack.top()->onExit();
        sceneStack.pop();
        if (!sceneStack.empty()) {
            sceneStack.top()->onEnter();
        }
    }
}

void SceneManager::update(float deltaTime) {
    performSceneChange();
    
    if (!sceneStack.empty()) {
        sceneStack.top()->update(deltaTime);
    }
}

void SceneManager::render(Graphics::Renderer* renderer) {
    if (!sceneStack.empty()) {
        sceneStack.top()->render(renderer);
    }
}

void SceneManager::handleInput(Input::InputManager* input) {
    if (!sceneStack.empty()) {
        sceneStack.top()->handleInput(input);
    }
}

Scene* SceneManager::getCurrentScene() {
    return !sceneStack.empty() ? sceneStack.top().get() : nullptr;
}

bool SceneManager::hasScene(const std::string& name) const {
    return scenes.find(name) != scenes.end();
}

void SceneManager::performSceneChange() {
    if (shouldChange && nextScene) {
        if (!sceneStack.empty()) {
            sceneStack.top()->onExit();
            sceneStack.pop();
        }
        sceneStack.push(nextScene);
        sceneStack.top()->onEnter();
        nextScene = nullptr;
        shouldChange = false;
    }
}

} // namespace Scene
} // namespace JJM
