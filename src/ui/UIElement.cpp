#include "ui/UIElement.h"
#include <algorithm>

namespace JJM {
namespace UI {

// UIElement implementation
UIElement::UIElement(const Math::Vector2D& pos, const Math::Vector2D& sz)
    : position(pos), size(sz), visible(true), enabled(true), parent(nullptr) {}

UIElement::~UIElement() {}

void UIElement::update(float deltaTime) {
    if (!visible || !enabled) return;
    
    for (auto& child : children) {
        child->update(deltaTime);
    }
}

void UIElement::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    for (auto& child : children) {
        child->render(renderer);
    }
}

void UIElement::handleInput(const Input::InputManager& input) {
    if (!visible || !enabled) return;
    
    for (auto& child : children) {
        child->handleInput(input);
    }
}

void UIElement::addChild(std::unique_ptr<UIElement> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

void UIElement::removeChild(UIElement* child) {
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [child](const std::unique_ptr<UIElement>& ptr) { return ptr.get() == child; }),
        children.end()
    );
}

Math::Vector2D UIElement::getAbsolutePosition() const {
    if (parent) {
        return parent->getAbsolutePosition() + position;
    }
    return position;
}

bool UIElement::containsPoint(const Math::Vector2D& point) const {
    Math::Vector2D absPos = getAbsolutePosition();
    return point.x >= absPos.x && point.x <= absPos.x + size.x &&
           point.y >= absPos.y && point.y <= absPos.y + size.y;
}

// Button implementation
Button::Button(const Math::Vector2D& pos, const Math::Vector2D& size, const std::string& txt)
    : UIElement(pos, size), text(txt),
      normalColor(Graphics::Color(100, 100, 100, 255)),
      hoverColor(Graphics::Color(150, 150, 150, 255)),
      pressedColor(Graphics::Color(80, 80, 80, 255)),
      textColor(Graphics::Color::White()),
      isHovered(false), isPressed(false) {}

void Button::update(float deltaTime) {
    UIElement::update(deltaTime);
}

void Button::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    Math::Vector2D absPos = getAbsolutePosition();
    Graphics::Color currentColor = normalColor;
    
    if (isPressed) {
        currentColor = pressedColor;
    } else if (isHovered) {
        currentColor = hoverColor;
    }
    
    renderer->drawRect(absPos, size, currentColor, true);
    renderer->drawRect(absPos, size, Graphics::Color::Black(), false);
    
    // Draw text (centered)
    // Note: Actual text rendering would require SDL_ttf integration
    
    UIElement::render(renderer);
}

void Button::handleInput(const Input::InputManager& input) {
    if (!visible || !enabled) return;
    
    Math::Vector2D mousePos = input.getMousePosition();
    isHovered = containsPoint(mousePos);
    
    if (isHovered && input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
        isPressed = true;
    }
    
    if (isPressed && input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
        isPressed = false;
        if (isHovered && onClick) {
            onClick();
        }
    }
    
    UIElement::handleInput(input);
}

void Button::setColors(const Graphics::Color& normal, const Graphics::Color& hover, const Graphics::Color& pressed) {
    normalColor = normal;
    hoverColor = hover;
    pressedColor = pressed;
}

// Label implementation
Label::Label(const Math::Vector2D& pos, const std::string& txt)
    : UIElement(pos, Math::Vector2D(100, 30)), text(txt),
      textColor(Graphics::Color::White()), fontSize(16) {}

void Label::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    // Note: Actual text rendering would require SDL_ttf integration
    // For now, just draw a rectangle placeholder
    Math::Vector2D absPos = getAbsolutePosition();
    renderer->drawRect(absPos, size, Graphics::Color(50, 50, 50, 128), true);
    
    UIElement::render(renderer);
}

// Panel implementation
Panel::Panel(const Math::Vector2D& pos, const Math::Vector2D& size)
    : UIElement(pos, size),
      backgroundColor(Graphics::Color(40, 40, 40, 200)),
      borderColor(Graphics::Color::White()),
      borderWidth(2) {}

void Panel::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    Math::Vector2D absPos = getAbsolutePosition();
    renderer->drawRect(absPos, size, backgroundColor, true);
    renderer->drawRect(absPos, size, borderColor, false);
    
    UIElement::render(renderer);
}

// Slider implementation
Slider::Slider(const Math::Vector2D& pos, float width, float min, float max)
    : UIElement(pos, Math::Vector2D(width, 20)),
      value(min), minValue(min), maxValue(max),
      trackColor(Graphics::Color(80, 80, 80, 255)),
      thumbColor(Graphics::Color(200, 200, 200, 255)),
      isDragging(false) {}

void Slider::update(float deltaTime) {
    UIElement::update(deltaTime);
}

void Slider::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    Math::Vector2D absPos = getAbsolutePosition();
    
    // Draw track
    Math::Vector2D trackPos = absPos + Math::Vector2D(0, size.y / 2 - 2);
    Math::Vector2D trackSize(size.x, 4);
    renderer->drawRect(trackPos, trackSize, trackColor, true);
    
    // Draw thumb
    float normalizedValue = (value - minValue) / (maxValue - minValue);
    float thumbX = absPos.x + normalizedValue * size.x;
    Math::Vector2D thumbPos(thumbX - 8, absPos.y);
    Math::Vector2D thumbSize(16, size.y);
    renderer->drawRect(thumbPos, thumbSize, thumbColor, true);
    
    UIElement::render(renderer);
}

void Slider::handleInput(const Input::InputManager& input) {
    if (!visible || !enabled) return;
    
    Math::Vector2D mousePos = input.getMousePosition();
    
    if (input.isMouseButtonPressed(SDL_BUTTON_LEFT) && containsPoint(mousePos)) {
        isDragging = true;
    }
    
    if (input.isMouseButtonReleased(SDL_BUTTON_LEFT)) {
        isDragging = false;
    }
    
    if (isDragging) {
        Math::Vector2D absPos = getAbsolutePosition();
        float normalizedValue = (mousePos.x - absPos.x) / size.x;
        normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
        setValue(minValue + normalizedValue * (maxValue - minValue));
    }
    
    UIElement::handleInput(input);
}

void Slider::setValue(float val) {
    float oldValue = value;
    value = std::max(minValue, std::min(maxValue, val));
    
    if (value != oldValue && onValueChanged) {
        onValueChanged(value);
    }
}

// Checkbox implementation
Checkbox::Checkbox(const Math::Vector2D& pos, float size)
    : UIElement(pos, Math::Vector2D(size, size)),
      checked(false),
      boxColor(Graphics::Color(100, 100, 100, 255)),
      checkColor(Graphics::Color::Green()) {}

void Checkbox::render(Graphics::Renderer* renderer) {
    if (!visible) return;
    
    Math::Vector2D absPos = getAbsolutePosition();
    renderer->drawRect(absPos, size, boxColor, true);
    renderer->drawRect(absPos, size, Graphics::Color::White(), false);
    
    if (checked) {
        Math::Vector2D checkSize = size * 0.6f;
        Math::Vector2D checkPos = absPos + (size - checkSize) * 0.5f;
        renderer->drawRect(checkPos, checkSize, checkColor, true);
    }
    
    UIElement::render(renderer);
}

void Checkbox::handleInput(const Input::InputManager& input) {
    if (!visible || !enabled) return;
    
    Math::Vector2D mousePos = input.getMousePosition();
    
    if (containsPoint(mousePos) && input.isMouseButtonPressed(SDL_BUTTON_LEFT)) {
        setChecked(!checked);
    }
    
    UIElement::handleInput(input);
}

void Checkbox::setChecked(bool ch) {
    bool oldValue = checked;
    checked = ch;
    
    if (checked != oldValue && onToggle) {
        onToggle(checked);
    }
}

} // namespace UI
} // namespace JJM
