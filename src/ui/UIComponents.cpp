#include "ui/UIComponents.h"
#include <algorithm>

namespace JJM {
namespace UI {

UIWidget::UIWidget()
    : rect(), visible(true), enabled(true), anchor(UIAnchor::TopLeft), hovered(false) {}

void UIWidget::update(float deltaTime) {
    (void)deltaTime;
}

void UIWidget::render() {}

bool UIWidget::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    (void)clicked;
    hovered = rect.contains(mousePos.x, mousePos.y);
    return false;
}

UIButton::UIButton(const std::string& text)
    : UIWidget(), text(text), pressed(false) {}

void UIButton::render() {
    if (!visible) return;
}

bool UIButton::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    hovered = rect.contains(mousePos.x, mousePos.y);
    
    if (hovered && clicked) {
        pressed = true;
        if (onClick) {
            onClick();
        }
        return true;
    }
    
    if (!clicked) {
        pressed = false;
    }
    
    return false;
}

UISlider::UISlider(float minValue, float maxValue)
    : UIWidget(), value(minValue), minValue(minValue), maxValue(maxValue), dragging(false) {}

void UISlider::render() {
    if (!visible) return;
}

bool UISlider::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    hovered = rect.contains(mousePos.x, mousePos.y);
    
    if (clicked && hovered) {
        dragging = true;
    }
    
    if (!clicked) {
        dragging = false;
    }
    
    if (dragging) {
        float normalizedX = (mousePos.x - rect.x) / rect.width;
        normalizedX = std::max(0.0f, std::min(1.0f, normalizedX));
        
        float newValue = minValue + normalizedX * (maxValue - minValue);
        if (newValue != value) {
            value = newValue;
            if (onValueChanged) {
                onValueChanged(value);
            }
        }
        return true;
    }
    
    return false;
}

void UISlider::setValue(float value) {
    this->value = std::max(minValue, std::min(maxValue, value));
}

UIProgressBar::UIProgressBar()
    : UIWidget(), progress(0.0f) {
    color[0] = 0.0f;
    color[1] = 1.0f;
    color[2] = 0.0f;
}

void UIProgressBar::render() {
    if (!visible) return;
}

void UIProgressBar::setProgress(float progress) {
    this->progress = std::max(0.0f, std::min(1.0f, progress));
}

void UIProgressBar::setColor(float r, float g, float b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

UITextField::UITextField(const std::string& placeholder)
    : UIWidget(), text(""), placeholder(placeholder), maxLength(-1), focused(false) {}

void UITextField::render() {
    if (!visible) return;
}

bool UITextField::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    hovered = rect.contains(mousePos.x, mousePos.y);
    
    if (clicked) {
        focused = hovered;
        return focused;
    }
    
    return focused;
}

UIPanel::UIPanel() : UIWidget() {}

void UIPanel::render() {
    if (!visible) return;
    
    for (auto& child : children) {
        if (child) {
            child->render();
        }
    }
}

bool UIPanel::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    for (auto it = children.rbegin(); it != children.rend(); ++it) {
        if ((*it)->handleInput(mousePos, clicked)) {
            return true;
        }
    }
    
    return false;
}

void UIPanel::addChild(std::shared_ptr<UIWidget> child) {
    if (child) {
        children.push_back(child);
    }
}

void UIPanel::removeChild(std::shared_ptr<UIWidget> child) {
    children.erase(
        std::remove(children.begin(), children.end(), child),
        children.end()
    );
}

void UIPanel::clearChildren() {
    children.clear();
}

UIScrollView::UIScrollView()
    : UIWidget(), scrollPos(0, 0), contentSize(0, 0), scrolling(false), scrollStartPos(0, 0) {}

void UIScrollView::render() {
    if (!visible) return;
}

bool UIScrollView::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    hovered = rect.contains(mousePos.x, mousePos.y);
    
    if (clicked && hovered) {
        scrolling = true;
        scrollStartPos = mousePos;
        return true;
    }
    
    if (!clicked) {
        scrolling = false;
    }
    
    if (scrolling) {
        Math::Vector2D delta = mousePos - scrollStartPos;
        scrollPos.x = std::max(0.0f, std::min(contentSize.x - rect.width, scrollPos.x - delta.x));
        scrollPos.y = std::max(0.0f, std::min(contentSize.y - rect.height, scrollPos.y - delta.y));
        scrollStartPos = mousePos;
        return true;
    }
    
    return false;
}

void UIScrollView::setContentSize(float width, float height) {
    contentSize.x = width;
    contentSize.y = height;
}

void UIScrollView::setScrollPosition(float x, float y) {
    scrollPos.x = x;
    scrollPos.y = y;
}

UIDropdown::UIDropdown()
    : UIWidget(), selectedIndex(-1), expanded(false) {}

void UIDropdown::render() {
    if (!visible) return;
}

bool UIDropdown::handleInput(const Math::Vector2D& mousePos, bool clicked) {
    if (!visible || !enabled) return false;
    
    hovered = rect.contains(mousePos.x, mousePos.y);
    
    if (clicked && hovered) {
        expanded = !expanded;
        return true;
    }
    
    if (expanded && clicked) {
        float itemHeight = 30.0f;
        for (size_t i = 0; i < options.size(); ++i) {
            UIRect itemRect(rect.x, rect.y + rect.height + i * itemHeight, rect.width, itemHeight);
            if (itemRect.contains(mousePos.x, mousePos.y)) {
                setSelectedIndex(static_cast<int>(i));
                expanded = false;
                return true;
            }
        }
    }
    
    return false;
}

void UIDropdown::addOption(const std::string& option) {
    options.push_back(option);
    if (selectedIndex == -1) {
        selectedIndex = 0;
    }
}

void UIDropdown::setSelectedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(options.size())) {
        selectedIndex = index;
        if (onSelectionChanged) {
            onSelectionChanged(selectedIndex);
        }
    }
}

const std::string& UIDropdown::getSelectedOption() const {
    static std::string empty;
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(options.size())) {
        return options[selectedIndex];
    }
    return empty;
}

} // namespace UI
} // namespace JJM
