#ifndef SCENE_H
#define SCENE_H

#include "ecs/EntityManager.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include <string>

namespace JJM {
namespace Scene {

class Scene {
protected:
    std::string name;
    ECS::EntityManager entityManager;
    bool initialized;

public:
    Scene(const std::string& sceneName);
    virtual ~Scene();

    virtual void onEnter() {}
    virtual void onExit() {}
    virtual void update(float deltaTime);
    virtual void render(Graphics::Renderer* renderer);
    virtual void handleInput(Input::InputManager* input) {}

    const std::string& getName() const { return name; }
    ECS::EntityManager* getEntityManager() { return &entityManager; }
    bool isInitialized() const { return initialized; }
};

} // namespace Scene
} // namespace JJM

#endif // SCENE_H
