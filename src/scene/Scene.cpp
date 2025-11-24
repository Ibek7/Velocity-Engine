#include "scene/Scene.h"

namespace JJM {
namespace Scene {

Scene::Scene(const std::string& sceneName) 
    : name(sceneName), initialized(false) {}

Scene::~Scene() {
    entityManager.clear();
}

void Scene::update(float deltaTime) {
    entityManager.update(deltaTime);
}

void Scene::render(Graphics::Renderer* renderer) {
    // Base rendering - override in derived classes
}

} // namespace Scene
} // namespace JJM
