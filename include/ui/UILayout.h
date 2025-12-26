#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

// UI Layout system with flexbox-like behavior
namespace Engine {

enum class LayoutDirection {
    Horizontal,
    Vertical
};

enum class LayoutAlignment {
    Start,
    Center,
    End,
    Stretch
};

enum class LayoutJustify {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

struct LayoutConstraints {
    float minWidth;
    float minHeight;
    float maxWidth;
    float maxHeight;
    float preferredWidth;
    float preferredHeight;
};

class UIElement;

class LayoutContainer {
public:
    LayoutContainer();
    ~LayoutContainer();

    // Configuration
    void setDirection(LayoutDirection dir) { m_direction = dir; }
    void setAlignment(LayoutAlignment align) { m_alignment = align; }
    void setJustify(LayoutJustify justify) { m_justify = justify; }
    void setPadding(float top, float right, float bottom, float left);
    void setGap(float gap) { m_gap = gap; }
    void setWrap(bool wrap) { m_wrap = wrap; }
    
    // Children management
    void addChild(UIElement* element, float flexGrow = 0.0f, float flexShrink = 1.0f);
    void removeChild(UIElement* element);
    void clearChildren();
    
    // Layout
    void layout(float availableWidth, float availableHeight);
    void getSize(float& width, float& height) const;
    
    // Query
    int getChildCount() const { return m_children.size(); }
    UIElement* getChild(int index) const;

private:
    struct ChildInfo {
        UIElement* element;
        float flexGrow;
        float flexShrink;
        float baseSize;
        float computedSize;
    };

    void computeLayout();
    float calculateTotalBaseSize() const;
    void distributeSpace(float availableSpace);
    void positionChildren();

    LayoutDirection m_direction;
    LayoutAlignment m_alignment;
    LayoutJustify m_justify;
    
    float m_paddingTop;
    float m_paddingRight;
    float m_paddingBottom;
    float m_paddingLeft;
    float m_gap;
    bool m_wrap;
    
    std::vector<ChildInfo> m_children;
    float m_computedWidth;
    float m_computedHeight;
};

class UIElement {
public:
    UIElement();
    virtual ~UIElement();

    // Transform
    void setPosition(float x, float y);
    void setSize(float width, float height);
    void getPosition(float& x, float& y) const;
    void getSize(float& width, float& height) const;
    
    // Constraints
    void setConstraints(const LayoutConstraints& constraints);
    const LayoutConstraints& getConstraints() const { return m_constraints; }
    
    // Visibility
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    // Layout container
    LayoutContainer* getLayoutContainer() { return m_layoutContainer.get(); }
    void enableLayout();
    
    // Virtual methods
    virtual void onLayout(float x, float y, float width, float height);
    virtual void render();

protected:
    float m_x;
    float m_y;
    float m_width;
    float m_height;
    
    LayoutConstraints m_constraints;
    bool m_visible;
    
    std::unique_ptr<LayoutContainer> m_layoutContainer;
};

class UILayoutSystem {
public:
    static UILayoutSystem& getInstance();

    // Root management
    void setRoot(UIElement* root);
    UIElement* getRoot() const { return m_root; }
    
    // Update
    void update(float viewportWidth, float viewportHeight);
    void render();
    
    // Registration
    void registerElement(const std::string& id, UIElement* element);
    void unregisterElement(const std::string& id);
    UIElement* getElementById(const std::string& id);
    
    // Layout queries
    void queryElementsInRect(float x, float y, float width, float height, 
                            std::vector<UIElement*>& results);

private:
    UILayoutSystem();
    UILayoutSystem(const UILayoutSystem&) = delete;
    UILayoutSystem& operator=(const UILayoutSystem&) = delete;

    UIElement* m_root;
    std::unordered_map<std::string, UIElement*> m_elements;
};

} // namespace Engine
