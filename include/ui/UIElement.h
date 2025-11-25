#ifndef UI_ELEMENT_H
#define UI_ELEMENT_H

#include "math/Vector2D.h"
#include "graphics/Color.h"
#include "graphics/Renderer.h"
#include "input/InputManager.h"
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace JJM {
namespace UI {

class UIElement {
protected:
    Math::Vector2D position;
    Math::Vector2D size;
    bool visible;
    bool enabled;
    std::string id;
    UIElement* parent;
    std::vector<std::unique_ptr<UIElement>> children;
    
public:
    UIElement(const Math::Vector2D& pos, const Math::Vector2D& sz);
    virtual ~UIElement();
    
    virtual void update(float deltaTime);
    virtual void render(Graphics::Renderer* renderer);
    virtual void handleInput(const Input::InputManager& input);
    
    void addChild(std::unique_ptr<UIElement> child);
    void removeChild(UIElement* child);
    
    Math::Vector2D getAbsolutePosition() const;
    bool containsPoint(const Math::Vector2D& point) const;
    
    void setPosition(const Math::Vector2D& pos) { position = pos; }
    void setSize(const Math::Vector2D& sz) { size = sz; }
    void setVisible(bool vis) { visible = vis; }
    void setEnabled(bool en) { enabled = en; }
    void setId(const std::string& identifier) { id = identifier; }
    
    const Math::Vector2D& getPosition() const { return position; }
    const Math::Vector2D& getSize() const { return size; }
    bool isVisible() const { return visible; }
    bool isEnabled() const { return enabled; }
    const std::string& getId() const { return id; }
};

class Button : public UIElement {
private:
    std::string text;
    Graphics::Color normalColor;
    Graphics::Color hoverColor;
    Graphics::Color pressedColor;
    Graphics::Color textColor;
    
    bool isHovered;
    bool isPressed;
    
    std::function<void()> onClick;
    
public:
    Button(const Math::Vector2D& pos, const Math::Vector2D& size, const std::string& txt);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setText(const std::string& txt) { text = txt; }
    void setOnClick(const std::function<void()>& callback) { onClick = callback; }
    void setColors(const Graphics::Color& normal, const Graphics::Color& hover, const Graphics::Color& pressed);
};

class Label : public UIElement {
private:
    std::string text;
    Graphics::Color textColor;
    int fontSize;
    
public:
    Label(const Math::Vector2D& pos, const std::string& txt);
    
    void render(Graphics::Renderer* renderer) override;
    
    void setText(const std::string& txt) { text = txt; }
    void setTextColor(const Graphics::Color& color) { textColor = color; }
    void setFontSize(int size) { fontSize = size; }
};

class Panel : public UIElement {
private:
    Graphics::Color backgroundColor;
    Graphics::Color borderColor;
    int borderWidth;
    
public:
    Panel(const Math::Vector2D& pos, const Math::Vector2D& size);
    
    void render(Graphics::Renderer* renderer) override;
    
    void setBackgroundColor(const Graphics::Color& color) { backgroundColor = color; }
    void setBorderColor(const Graphics::Color& color) { borderColor = color; }
    void setBorderWidth(int width) { borderWidth = width; }
};

class Slider : public UIElement {
private:
    float value;
    float minValue;
    float maxValue;
    Graphics::Color trackColor;
    Graphics::Color thumbColor;
    bool isDragging;
    
    std::function<void(float)> onValueChanged;
    
public:
    Slider(const Math::Vector2D& pos, float width, float min, float max);
    
    void update(float deltaTime) override;
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setValue(float val);
    float getValue() const { return value; }
    void setOnValueChanged(const std::function<void(float)>& callback) { onValueChanged = callback; }
};

class Checkbox : public UIElement {
private:
    bool checked;
    Graphics::Color boxColor;
    Graphics::Color checkColor;
    std::function<void(bool)> onToggle;
    
public:
    Checkbox(const Math::Vector2D& pos, float size);
    
    void render(Graphics::Renderer* renderer) override;
    void handleInput(const Input::InputManager& input) override;
    
    void setChecked(bool ch);
    bool isChecked() const { return checked; }
    void setOnToggle(const std::function<void(bool)>& callback) { onToggle = callback; }
};

} // namespace UI
} // namespace JJM

#endif // UI_ELEMENT_H
