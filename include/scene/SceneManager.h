#ifndef SCENE_MANAGER_H
#define SCENE_MANAGER_H

#include "scene/Scene.h"
#include <memory>
#include <unordered_map>
#include <stack>

namespace JJM {
namespace Scene {

class SceneManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Scene>> scenes;
    std::stack<std::shared_ptr<Scene>> sceneStack;
    std::shared_ptr<Scene> nextScene;
    bool shouldChange;

public:
    SceneManager();
    ~SceneManager();

    // Scene management
    void addScene(const std::string& name, std::shared_ptr<Scene> scene);
    void removeScene(const std::string& name);
    void changeScene(const std::string& name);
    void pushScene(const std::string& name);
    void popScene();

    // Update and render
    void update(float deltaTime);
    void render(Graphics::Renderer* renderer);
    void handleInput(Input::InputManager* input);

    // Getters
    Scene* getCurrentScene();
    bool hasScene(const std::string& name) const;

private:
    void performSceneChange();
};

} // namespace Scene
} // namespace JJM

#endif // SCENE_MANAGER_H
