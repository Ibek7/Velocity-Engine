#include "scene/SceneGraph.h"
#include <algorithm>
#include <cmath>

namespace JJM {
namespace Scene {

// SceneNode implementation
SceneNode::SceneNode(const std::string& name)
    : name(name), localPosition(0, 0), localRotation(0), localScale(1, 1),
      transformDirty(true), parent(nullptr), visible(true), active(true) {
}

void SceneNode::addChild(std::shared_ptr<SceneNode> child) {
    if (!child) return;
    
    if (child->parent) {
        child->removeFromParent();
    }
    
    child->parent = this;
    children.push_back(child);
    child->markDirty();
}

void SceneNode::removeChild(std::shared_ptr<SceneNode> child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->parent = nullptr;
        children.erase(it);
    }
}

void SceneNode::removeFromParent() {
    if (parent) {
        parent->removeChild(shared_from_this());
    }
}

std::shared_ptr<SceneNode> SceneNode::findChild(const std::string& name) const {
    for (const auto& child : children) {
        if (child->getName() == name) {
            return child;
        }
        auto result = child->findChild(name);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

Math::Vector2D SceneNode::getWorldPosition() const {
    const auto& transform = getWorldTransform();
    return Math::Vector2D(transform.m[0][2], transform.m[1][2]);
}

float SceneNode::getWorldRotation() const {
    float rot = localRotation;
    SceneNode* p = parent;
    while (p) {
        rot += p->localRotation;
        p = p->parent;
    }
    return rot;
}

Math::Vector2D SceneNode::getWorldScale() const {
    Math::Vector2D scale = localScale;
    SceneNode* p = parent;
    while (p) {
        scale.x *= p->localScale.x;
        scale.y *= p->localScale.y;
        p = p->parent;
    }
    return scale;
}

void SceneNode::markDirty() {
    transformDirty = true;
    for (auto& child : children) {
        child->markDirty();
    }
}

void SceneNode::updateTransform() const {
    if (!transformDirty) return;
    
    // Build local transform
    float c = std::cos(localRotation);
    float s = std::sin(localRotation);
    
    localTransform.m[0][0] = c * localScale.x;
    localTransform.m[0][1] = -s * localScale.x;
    localTransform.m[0][2] = localPosition.x;
    
    localTransform.m[1][0] = s * localScale.y;
    localTransform.m[1][1] = c * localScale.y;
    localTransform.m[1][2] = localPosition.y;
    
    localTransform.m[2][0] = 0;
    localTransform.m[2][1] = 0;
    localTransform.m[2][2] = 1;
    
    // Calculate world transform
    if (parent) {
        worldTransform = parent->getWorldTransform() * localTransform;
    } else {
        worldTransform = localTransform;
    }
    
    transformDirty = false;
}

const Math::Matrix3x3& SceneNode::getLocalTransform() const {
    updateTransform();
    return localTransform;
}

const Math::Matrix3x3& SceneNode::getWorldTransform() const {
    updateTransform();
    return worldTransform;
}

void SceneNode::update(float deltaTime) {
    if (!active) return;
    
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

void SceneNode::render() {
    if (!visible) return;
    
    for (auto& child : children) {
        child->render();
    }
}

// SceneGraph implementation
SceneGraph::SceneGraph()
    : root(std::make_shared<SceneNode>("Root")) {
}

void SceneGraph::update(float deltaTime) {
    if (root) {
        updateNode(root.get(), deltaTime);
    }
}

void SceneGraph::render() {
    if (root) {
        renderNode(root.get());
    }
}

void SceneGraph::updateNode(SceneNode* node, float deltaTime) {
    if (!node || !node->isActive()) return;
    
    node->update(deltaTime);
}

void SceneGraph::renderNode(SceneNode* node) {
    if (!node || !node->isVisible()) return;
    
    node->render();
}

std::shared_ptr<SceneNode> SceneGraph::findNode(const std::string& name) const {
    if (!root) return nullptr;
    if (root->getName() == name) return root;
    return findNodeRecursive(root.get(), name);
}

std::shared_ptr<SceneNode> SceneGraph::findNodeRecursive(SceneNode* node, const std::string& name) const {
    for (const auto& child : node->getChildren()) {
        if (child->getName() == name) {
            return child;
        }
        auto result = findNodeRecursive(child.get(), name);
        if (result) {
            return result;
        }
    }
    return nullptr;
}

void SceneGraph::traverse(std::function<void(SceneNode*)> callback) {
    if (root) {
        traverseNode(root.get(), callback);
    }
}

void SceneGraph::traverseNode(SceneNode* node, std::function<void(SceneNode*)> callback) {
    callback(node);
    for (const auto& child : node->getChildren()) {
        traverseNode(child.get(), callback);
    }
}

std::vector<std::shared_ptr<SceneNode>> SceneGraph::findNodesWithTag(const std::string&) const {
    // Stub - would need tag system
    return {};
}

} // namespace Scene
} // namespace JJM
