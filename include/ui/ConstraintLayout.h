#ifndef CONSTRAINT_LAYOUT_H
#define CONSTRAINT_LAYOUT_H

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

namespace JJM {
namespace UI {

enum class Anchor {
    TOP, BOTTOM, LEFT, RIGHT,
    CENTER_X, CENTER_Y,
    TOP_LEFT, TOP_RIGHT,
    BOTTOM_LEFT, BOTTOM_RIGHT,
    CENTER
};

enum class SizeMode {
    FIXED,           // Fixed size in pixels
    PERCENT,         // Percentage of parent
    WRAP_CONTENT,    // Fit to content
    MATCH_PARENT     // Fill parent
};

struct Constraint {
    std::string targetId;
    Anchor targetAnchor;
    Anchor sourceAnchor;
    float offset;
    
    Constraint() : offset(0) {}
};

struct LayoutParams {
    SizeMode widthMode = SizeMode::FIXED;
    SizeMode heightMode = SizeMode::FIXED;
    float width = 100;
    float height = 100;
    
    std::vector<Constraint> constraints;
    
    float minWidth = 0, minHeight = 0;
    float maxWidth = 10000, maxHeight = 10000;
    float aspectRatio = 0;  // 0 = no constraint
    
    float marginTop = 0, marginBottom = 0;
    float marginLeft = 0, marginRight = 0;
    float paddingTop = 0, paddingBottom = 0;
    float paddingLeft = 0, paddingRight = 0;
};

class UIElement {
public:
    UIElement(const std::string& id);
    virtual ~UIElement() = default;
    
    void setId(const std::string& id) { m_id = id; }
    const std::string& getId() const { return m_id; }
    
    void setPosition(float x, float y) { m_x = x; m_y = y; }
    void setSize(float width, float height) { m_width = width; m_height = height; }
    
    float getX() const { return m_x; }
    float getY() const { return m_y; }
    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    
    void setLayoutParams(const LayoutParams& params) { m_layoutParams = params; }
    LayoutParams& getLayoutParams() { return m_layoutParams; }
    
    void setParent(UIElement* parent) { m_parent = parent; }
    UIElement* getParent() { return m_parent; }
    
    void addChild(std::shared_ptr<UIElement> child);
    void removeChild(const std::string& childId);
    const std::vector<std::shared_ptr<UIElement>>& getChildren() const { return m_children; }
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    virtual void layout();
    virtual void render() = 0;
    
protected:
    std::string m_id;
    float m_x, m_y;
    float m_width, m_height;
    LayoutParams m_layoutParams;
    UIElement* m_parent;
    std::vector<std::shared_ptr<UIElement>> m_children;
    bool m_visible;
};

class ConstraintSolver {
public:
    void solve(UIElement* root);
    
private:
    void resolveConstraints(UIElement* element, 
                          std::unordered_map<std::string, UIElement*>& elements);
    float getAnchorPosition(UIElement* element, Anchor anchor, bool isHorizontal);
};

} // namespace UI
} // namespace JJM

#endif
