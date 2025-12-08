#pragma once

#include "math/Vector2D.h"
#include "graphics/Texture.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace JJM {
namespace UI {

enum class UIAlignment {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

enum class UIAnchor {
    TopLeft, TopCenter, TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight,
    Stretch
};

struct UIRect {
    float x, y, width, height;
    
    UIRect() : x(0), y(0), width(0), height(0) {}
    UIRect(float x, float y, float w, float h) : x(x), y(y), width(w), height(h) {}
    
    bool contains(float px, float py) const {
        return px >= x && px <= x + width && py >= y && py <= y + height;
    }
};

class UIWidget {
public:
    UIWidget();
    virtual ~UIWidget() = default;
    
    virtual void update(float deltaTime);
    virtual void render();
    virtual bool handleInput(const Math::Vector2D& mousePos, bool clicked);
    
    void setPosition(float x, float y) { rect.x = x; rect.y = y; }
    void setSize(float w, float h) { rect.width = w; rect.height = h; }
    void setRect(const UIRect& r) { rect = r; }
    
    const UIRect& getRect() const { return rect; }
    
    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }
    
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }
    
    void setAnchor(UIAnchor anchor) { this->anchor = anchor; }
    UIAnchor getAnchor() const { return anchor; }

protected:
    UIRect rect;
    bool visible;
    bool enabled;
    UIAnchor anchor;
    bool hovered;
};

class UIButton : public UIWidget {
public:
    UIButton(const std::string& text);
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void setText(const std::string& text) { this->text = text; }
    const std::string& getText() const { return text; }
    
    void setOnClick(std::function<void()> callback) { onClick = callback; }

private:
    std::string text;
    std::function<void()> onClick;
    bool pressed;
};

class UISlider : public UIWidget {
public:
    UISlider(float minValue, float maxValue);
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void setValue(float value);
    float getValue() const { return value; }
    
    void setMinValue(float min) { minValue = min; }
    void setMaxValue(float max) { maxValue = max; }
    
    void setOnValueChanged(std::function<void(float)> callback) { onValueChanged = callback; }

private:
    float value;
    float minValue;
    float maxValue;
    bool dragging;
    std::function<void(float)> onValueChanged;
};

class UIProgressBar : public UIWidget {
public:
    UIProgressBar();
    
    void render() override;
    
    void setProgress(float progress);
    float getProgress() const { return progress; }
    
    void setColor(float r, float g, float b);

private:
    float progress;
    float color[3];
};

class UITextField : public UIWidget {
public:
    UITextField(const std::string& placeholder = "");
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void setText(const std::string& text) { this->text = text; }
    const std::string& getText() const { return text; }
    
    void setPlaceholder(const std::string& placeholder) { this->placeholder = placeholder; }
    
    void setMaxLength(int maxLength) { this->maxLength = maxLength; }
    
    void setOnTextChanged(std::function<void(const std::string&)> callback) { onTextChanged = callback; }

private:
    std::string text;
    std::string placeholder;
    int maxLength;
    bool focused;
    std::function<void(const std::string&)> onTextChanged;
};

class UIPanel : public UIWidget {
public:
    UIPanel();
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void addChild(std::shared_ptr<UIWidget> child);
    void removeChild(std::shared_ptr<UIWidget> child);
    void clearChildren();
    
    const std::vector<std::shared_ptr<UIWidget>>& getChildren() const { return children; }

private:
    std::vector<std::shared_ptr<UIWidget>> children;
};

class UIScrollView : public UIWidget {
public:
    UIScrollView();
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void setContentSize(float width, float height);
    void setScrollPosition(float x, float y);
    
    Math::Vector2D getScrollPosition() const { return scrollPos; }

private:
    Math::Vector2D scrollPos;
    Math::Vector2D contentSize;
    bool scrolling;
    Math::Vector2D scrollStartPos;
};

class UIDropdown : public UIWidget {
public:
    UIDropdown();
    
    void render() override;
    bool handleInput(const Math::Vector2D& mousePos, bool clicked) override;
    
    void addOption(const std::string& option);
    void setSelectedIndex(int index);
    int getSelectedIndex() const { return selectedIndex; }
    
    const std::string& getSelectedOption() const;
    
    void setOnSelectionChanged(std::function<void(int)> callback) { onSelectionChanged = callback; }

private:
    std::vector<std::string> options;
    int selectedIndex;
    bool expanded;
    std::function<void(int)> onSelectionChanged;
};

} // namespace UI
} // namespace JJM
