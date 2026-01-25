#include "animation/MorphTargetSystem.h"
#include <algorithm>

namespace JJM {
namespace Animation {

MorphTargetController::MorphTargetController() {
}

MorphTargetController::~MorphTargetController() {
}

void MorphTargetController::addMorphTarget(const std::string& name, const MorphTarget& target) {
    m_targetIndices[name] = static_cast<int>(m_targets.size());
    m_targets.push_back(target);
}

void MorphTargetController::setWeight(const std::string& name, float weight) {
    auto it = m_targetIndices.find(name);
    if (it != m_targetIndices.end()) {
        m_targets[it->second].weight = std::max(0.0f, std::min(1.0f, weight));
    }
}

float MorphTargetController::getWeight(const std::string& name) const {
    auto it = m_targetIndices.find(name);
    return it != m_targetIndices.end() ? m_targets[it->second].weight : 0.0f;
}

void MorphTargetController::update(float deltaTime) {
    // Update weights, could animate them
}

void MorphTargetController::applyToMesh() {
    // Apply all morph targets to mesh vertices
    // This would modify vertex buffer based on weights
}

void BlendShapeSystem::registerController(MorphTargetController* controller) {
    m_controllers.push_back(controller);
}

void BlendShapeSystem::unregisterController(MorphTargetController* controller) {
    m_controllers.erase(
        std::remove(m_controllers.begin(), m_controllers.end(), controller),
        m_controllers.end()
    );
}

void BlendShapeSystem::update(float deltaTime) {
    for (auto* controller : m_controllers) {
        controller->update(deltaTime);
    }
}

} // namespace Animation
} // namespace JJM
