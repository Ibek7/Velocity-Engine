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
    Math::Matrix3x3 getWorldToLocalMatrix() const;
    
    // Transform operations
    void translate(const Math::Vector2D& delta);
    void rotate(float angleDelta);
    void scaleBy(const Math::Vector2D& factor);
    Math::Vector2D transformPoint(const Math::Vector2D& point) const;  // Local to world
    Math::Vector2D inverseTransformPoint(const Math::Vector2D& point) const;  // World to local
    
    // Hierarchy
    void addChild(std::shared_ptr<SceneNode> child);
    void removeChild(std::shared_ptr<SceneNode> child);
    void removeFromParent();
    void removeAllChildren();
    std::shared_ptr<SceneNode> findChild(const std::string& name) const;
    std::vector<std::shared_ptr<SceneNode>> getChildrenRecursive() const;
    int getDepth() const;  // Distance from root
    
    SceneNode* getParent() const { return parent; }
    const std::vector<std::shared_ptr<SceneNode>>& getChildren() const { return children; }
    size_t getChildCount() const { return children.size(); }
    bool hasChildren() const { return !children.empty(); }
    
    // Transform propagation
    void propagateTransform();  // Force update of entire subtree
    void invalidateWorldTransform();  // Mark this and all children dirty
    
    // Visibility and activity
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    bool isVisibleInHierarchy() const;  // Check parent visibility too
    void setActive(bool active) { this->active = active; }
    bool isActive() const { return active; }
    bool isActiveInHierarchy() const;  // Check parent active state too
    
    // Update and render
    virtual void update(float deltaTime);
    virtual void render();
    
    // Transform matrix
    const Math::Matrix3x3& getLocalTransform() const;
    const Math::Matrix3x3& getWorldTransform() const;
    
    // Name and tags
    const std::string& getName() const { return name; }
    void setName(const std::string& name) { this->name = name; }
    void addTag(const std::string& tag) { tags.push_back(tag); }
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::vector<std::string>& getTags() const { return tags; }
    
protected:
    void markDirty();
    void updateTransform() const;
    void onTransformChanged();  // Override for custom behavior
    
    std::string name;
    std::vector<std::string> tags;
    Math::Vector2D localPosition;
    float localRotation;
    Math::Vector2D localScale;
    
    mutable Math::Matrix3x3 localTransform;
    mutable Math::Matrix3x3 worldTransform;
    mutable bool transformDirty;
    mutable bool worldTransformDirty;
    
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
