#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include "math/Vector2D.h"
#include "math/Matrix3x3.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace JJM {
namespace Scene {

class SceneNode : public std::enable_shared_from_this<SceneNode> {
public:
    SceneNode(const std::string& name = "");
    virtual ~SceneNode() = default;
    
    // Transform
    void setPosition(const Math::Vector2D& pos) { localPosition = pos; markDirty(); }
    void setRotation(float angle) { localRotation = angle; markDirty(); }
    void setScale(const Math::Vector2D& scale) { localScale = scale; markDirty(); }
    
    Math::Vector2D getPosition() const { return localPosition; }
    float getRotation() const { return localRotation; }
    Math::Vector2D getScale() const { return localScale; }
    
    Math::Vector2D getWorldPosition() const;
    float getWorldRotation() const;
    Math::Vector2D getWorldScale() const;
    
    // Hierarchy
    void addChild(std::shared_ptr<SceneNode> child);
    void removeChild(std::shared_ptr<SceneNode> child);
    void removeFromParent();
    std::shared_ptr<SceneNode> findChild(const std::string& name) const;
    
    SceneNode* getParent() const { return parent; }
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const { return children; }
    
    // Visibility and activity
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    void setActive(bool active) { this->active = active; }
    bool isActive() const { return active; }
    
    // Update and render
    virtual void update(float deltaTime);
    virtual void render();
    
    // Transform matrix
    const Math::Matrix3x3& getLocalTransform() const;
    const Math::Matrix3x3& getWorldTransform() const;
    
    // Name
    const std::string& getName() const { return name; }
    void setName(const std::string& name) { this->name = name; }
    
protected:
    void markDirty();
    void updateTransform() const;
    
    std::string name;
    Math::Vector2D localPosition;
    float localRotation;
    Math::Vector2D localScale;
    
    mutable Math::Matrix3x3 localTransform;
    mutable Math::Matrix3x3 worldTransform;
    mutable bool transformDirty;
    
    SceneNode* parent;
    std::vector<std::shared_ptr<SceneNode>> children;
    
    bool visible;
    bool active;
};

class SceneGraph {
public:
    SceneGraph();
    ~SceneGraph() = default;
    
    void setRoot(std::shared_ptr<SceneNode> root) { this->root = root; }
    std::shared_ptr<SceneNode> getRoot() const { return root; }
    
    void update(float deltaTime);
    void render();
    
    // Node search
    std::shared_ptr<SceneNode> findNode(const std::string& name) const;
    std::vector<std::shared_ptr<SceneNode>> findNodesWithTag(const std::string& tag) const;
    
    // Traversal
    void traverse(std::function<void(SceneNode*)> callback);
    
private:
    std::shared_ptr<SceneNode> root;
    
    void updateNode(SceneNode* node, float deltaTime);
    void renderNode(SceneNode* node);
    void traverseNode(SceneNode* node, std::function<void(SceneNode*)> callback);
    std::shared_ptr<SceneNode> findNodeRecursive(SceneNode* node, const std::string& name) const;
};

} // namespace Scene
} // namespace JJM

#endif // SCENE_GRAPH_H
